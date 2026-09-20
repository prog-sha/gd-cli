/**************************************************************************/
/*  sink.cpp                                                              */
/**************************************************************************/
/*                          Command-line runtime                         */
/**************************************************************************/

// Implement incremental file writing declared in sink.h.

#include "cli/sys/sink.h"

#include "cli/sys/file_job.h"
#include "cli/sys/os.h"
#include "cli/sys/wait.h"

// Write queued chunks on a worker until the queue is empty.
void FileSinkJob::run() {
	if (sink.is_valid()) {
		sink->drain_on_worker();
	}
}

// Release references on the main thread; callers query Sink for the result.
void FileSinkJob::finish() {
	if (sink.is_valid()) {
		sink->job_finished();
	}
	sink.unref();
}

// Wake pending body processing after worker progress.
void FileSink::job_finished() {
	if (ready.is_valid()) {
		ready.call();
	}
}

// Begin opening the destination on a worker.
void FileSink::open(const String &p_path, bool p_want_hash) {
	{
		MutexLock lock(mutex);
		path = p_path;
		want_hash = p_want_hash;
	}
	kick();
}

// Accept a chunk and preserve submission order.
void FileSink::push(const PackedByteArray &p_chunk) {
	if (p_chunk.is_empty()) {
		return;
	}
	{
		MutexLock lock(mutex);
		if (closing || closed || !why.is_empty()) {
			return; // Already closed or failed.
		}
		queue.push_back(p_chunk);
		queued_bytes += p_chunk.size();
	}
	kick();
}

// Signal that no more chunks will arrive.
void FileSink::close() {
	{
		MutexLock lock(mutex);
		if (closing) {
			return;
		}
		closing = true;
	}
	kick(); // Close only after writing the remainder.
}

// Schedule one worker if none is running.
// Keep exactly one writer active for each sink.
void FileSink::kick() {
	{
		MutexLock lock(mutex);
		if (running || closed) {
			return;
		}
		if (queue.is_empty() && !closing && opened) {
			return; // No data to write and no closure requested.
		}
		running = true;
	}
	Ref<FileSinkJob> job;
	job.instantiate();
	job->sink = Ref<FileSink>(this);
	job->submit();
}

// Access the file only on workers and release the shared mutex during OS waits.
void FileSink::drain_on_worker() {
	for (;;) {
		PackedByteArray chunk;
		String target;
		bool need_open = false, finish = false, digest = false;
		{
			MutexLock lock(mutex);
			finish = aborted || (opened && closing && queue.is_empty());
			need_open = !finish && !opened;
			if (need_open) { target = path; digest = want_hash; }
			else if (!finish && !queue.is_empty()) {
				chunk = queue.front()->get();
				queue.pop_front();
				queued_bytes -= chunk.size();
			} else if (!finish) {
				running = false;
				return;
			}
		}
		if (need_open) {
			file = GDFile::open(target, GDFile::WRITE);
			bool ok = file.is_valid();
			if (ok && digest) { hash.instantiate(); ok = hash->start() == OK; }
			if (ok) {
				MutexLock lock(mutex);
				opened = true;
				continue;
			}
			{ MutexLock lock(mutex); why = vformat("cannot open %s", target); }
			finish = true;
		}
		if (!finish) {
			const bool ok = file->store_buffer(chunk) && (hash.is_null() || hash->update(chunk) == OK);
			if (ok) continue;
			{ MutexLock lock(mutex); why = "cannot write the file"; }
		}
		// Report close failures and delete partial files only after closing.
		const Error error = file.is_valid() ? file->close() : OK;
		file.unref();
		String drop;
		{
			MutexLock lock(mutex);
			if (error != OK && why.is_empty()) why = "cannot close the file";
			queue.clear();
			queued_bytes = 0;
			closed = true;
			running = false;
			drop = drop_path;
			drop_path.clear();
		}
		if (!drop.is_empty()) Os::remove(drop);
		return;
	}
}

// Report whether writing is complete.
bool FileSink::done() {
	MutexLock lock(mutex);
	return closed;
}

// Return bytes retained in the pending queue.
int64_t FileSink::pending_bytes() {
	MutexLock lock(mutex);
	return queued_bytes;
}

// Return the failure detail.
String FileSink::error() {
	MutexLock lock(mutex);
	return why;
}

// Return the written content's SHA-256 in hexadecimal.
// Read only after closed; the worker owns the hash until closure.
String FileSink::digest() {
	MutexLock lock(mutex);
	if (hash.is_null() || !closed || running) {
		return String();
	}
	const PackedByteArray raw = hash->finish();
	hash.unref();
	String out;
	for (int i = 0; i < raw.size(); i++) {
		out += String::num_uint64(raw[i] >> 4, 16) + String::num_uint64(raw[i] & 0xf, 16);
	}
	return out;
}

// Abort writing and remove the partial file after worker-side closure.
void FileSink::abort(const String &p_drop_path) {
	bool drop_later = false;
	{
		MutexLock lock(mutex);
		queue.clear();
		queued_bytes = 0;
		closing = true;
		aborted = true;
		drop_path = p_drop_path;
		if (closed && !running) {
			drop_later = !drop_path.is_empty();
			drop_path = String();
		}
	}
	if (drop_later) {
		GDFileCall::start([p_drop_path]() { return Os::remove(p_drop_path); });
	} else {
		kick(); // Delegate file cleanup to the worker.
	}
}
