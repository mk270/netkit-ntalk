# Changes in this fork

Relative to the Debian package `netkit-ntalk` 0.17-19 (git tag
`debian/0.17-19`). Branch `fixes`, oldest first.

* **Add a talkd loopback test harness** (`tests/run.sh`): ASan and
  UBSan, plus MSan when clang is available. `MAX_LIFE` can be
  overridden at build time so tests need not wait a minute.
* **Fix use-after-free in `expire()`** (remote crash).
* **Zero the reply before filling it in** (uninitialised bytes were
  sent).
* **Cap the invitation table at 1024 entries.**
* **Fix off-by-one in debug type/answer names.**
* **Bound utmp tty names when filling `r_tty`.**
* **Check the announce target is the callee's tty** (terminal, owned
  by the callee, opened non-blocking).
* **Make libwrap optional.** CMake option `USE_LIBWRAP` (default on;
  configuration fails if the library is missing), `configure`
  auto-detects or takes `--without-libwrap`. Without it, the peer's
  name is shown only if it resolves back to the sender's address, else
  the numeric address. The CMake change is untested.
* **Clean up `-Wall -Wextra` warnings.** An unused variable; ignored
  `write()` results made explicit; a verified host name too long for
  its buffer now falls back to the numeric address instead of being
  truncated.
* **Add CI** (GitHub Actions): hardened CMake build and the sanitizer
  tests. Not yet run on GitHub.

Details and impact of each security fix are in SECURITY.md.

## Relative to upstream 0.17

The first Debian patch (a squash of many older patches) added libwrap
access control in place of a double reverse-DNS check, a source-port
check, a `-q` option to quiet logging, and compatibility code for
SunOS's different packet format, among other changes. Two further
patches add and adjust a CMake build. The `debian` branch has them as
commits; see PROVENANCE.md.
