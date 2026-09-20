# Install the signed universal executable and its license information.
class GdCli < Formula
  desc "Command-line runtime for typed scripts, services, and data workflows"
  homepage "https://gd-cli.progsha.com/"
  url "https://github.com/prog-sha/gd-cli/releases/download/v0.7.3/gd-macos-universal.zip"
  version "0.7.3"
  sha256 "a1cf389162dbb59c14e8945ec036365892f0d46bc29acb751888ea0468f742cd"
  license "MIT"
  depends_on :macos
  depends_on macos: :high_sierra
  skip_clean "bin/gd" # Preserve the executable signature.

  # Install the published executable without rewriting its load commands.
  def install
    bin.install "gd"
    pkgshare.install "LICENSE.txt", "COPYRIGHT.txt", "NOTICE.md"
  end

  # Confirm startup and script evaluation after installation.
  test do
    assert_match version.to_s, shell_output("#{bin}/gd --version")
    assert_equal "42", shell_output("#{bin}/gd eval 'print(6 * 7)' ").strip
  end
end
