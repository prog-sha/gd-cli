# Verify loopback HTTP request conversion and serving without a scene tree.
extends RefCounted


# Decode a body through the ordinary automatic-wait API.
func echo(req: GDWebRequest) -> Dictionary:
	var value, failure := req.json()
	return GD.web.text("invalid", 400) if failure != null else GD.web.json(value)


# Retain the explicit asynchronous body entry point and its Signal contract.
func later(req: GDWebRequest) -> Dictionary:
	var pending: Variant = req.json_async()
	if not pending is Signal:
		return GD.web.text("expected Signal", 500)
	var value: R = await pending
	return GD.web.text("invalid", 400) if not value.ok else GD.web.json(value.v)


# Complete all loopback requests, stop the listener, and terminate the serving loop.
func main() -> int:
	var ck := GD.test.check()
	var loop := Engine.get_main_loop()
	ck.no(loop is SceneTree, "serve starts without a tree")
	if loop.has_method("has_scene_tree"):
		ck.no(bool(loop.call("has_scene_tree")), "no lazy tree at startup")
	var app := GD.web.app()
	app.route("POST", "/echo", echo)
	app.route("POST", "/async", later)
	var opened: R = app.listen(0, "127.0.0.1")
	ck.succeeds(opened, "listen on an assigned port")
	if opened.ok:
		var base := "http://127.0.0.1:%d" % app.port()
		for route: String in ["/echo", "/async"]:
			var reply: GDHTTPResponse = GD.http.fetch(base + route, {"method": "POST", "body": '{"text":"日本😀"}', "headers": {"Content-Type": "application/json"}})
			ck.ok(reply.error.is_empty(), "HTTP request completes")
			if reply.error.is_empty():
				ck.eq(reply.status, 200, "JSON status")
				ck.eq(GD.data.json_decode(reply.body).v, {"text": "日本😀"}, "JSON body")
		var bad: GDHTTPResponse = GD.http.fetch(base + "/echo", {"method": "POST", "body": "{"})
		ck.ok(bad.error.is_empty(), "malformed JSON response")
		if bad.error.is_empty():
			ck.eq(bad.status, 400, "malformed JSON status")
		var missing: GDHTTPResponse = GD.http.fetch(base + "/missing")
		ck.ok(missing.error.is_empty(), "missing route response")
		if missing.error.is_empty():
			ck.eq(missing.status, 404, "missing route status")
	app.stop()
	if loop.has_method("has_scene_tree"):
		ck.no(bool(loop.call("has_scene_tree")), "serving did not create a tree")
	print("release:web:%d" % ck.code())
	loop.call("quit", ck.code())
	return ck.code()
