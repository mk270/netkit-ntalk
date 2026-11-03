# netkit-ntalk (unofficial fork)

`talk`, the classic two-person terminal chat client, and `talkd`, the
daemon that sets up talk sessions between machines. This repository is
an **unofficial fork** of the Debian package `netkit-ntalk` 0.17-19. It
is not affiliated with the Netkit authors, the University of
California, or Debian. The upstream README is kept as
[`README`](README).

## Why this exists

Netkit-ntalk is unmaintained and the Debian package is orphaned
(Debian QA group). This fork preserves the code with its history,
fixes bugs found in a security review (one lets any remote host crash
`talkd`), and makes the libwrap dependency optional. See
[CHANGES.md](CHANGES.md) and [SECURITY.md](SECURITY.md).

## Building

You need a C compiler and the ncurses development files.

Using CMake (the build Debian uses; **not tested here**, because
cmake was not available):

    cmake -S . -B build              # needs libwrap0-dev, or add:
    cmake -S . -B build -DUSE_LIBWRAP=OFF
    cmake --build build

Using the original `configure` and `make` (tested):

    ./configure                      # uses libwrap if it finds it
    ./configure --without-libwrap    # or force it off
    make

The programs are `talk` (CMake names it `netkit-ntalk`) and `talkd`
(`in.ntalkd` with CMake). `talkd` expects its UDP socket on standard
input, as when started by inetd. It runs with the privileges needed
to write to users' terminals, normally root.

## libwrap

With libwrap, `talkd` consults `/etc/hosts.allow` and `/etc/hosts.deny`
to decide which hosts may send it requests. Without it, `talkd`
accepts requests from any host, so restrict access with a firewall. In
that case the sender's host name appears in the announcement only if
it resolves back to the sender's address; otherwise the numeric address
is shown.

## Tests

`tests/run.sh` builds `talkd` with AddressSanitizer and
UndefinedBehaviorSanitizer (and MemorySanitizer if `clang` is
installed) and drives it over loopback UDP. It needs `python3`. It also
builds the libwrap code path against a permissive test-only stand-in
for `tcpd.h`, which is never part of a real build. Not covered:
announcing to a real terminal, and the `talk` client.

## Branches and tags

| Ref | Contents |
| --- | --- |
| `master` | upstream 0.17 tarball, unmodified (one commit) |
| tag `upstream/0.17` | the same commit |
| `debian` | master plus Debian's patches as commits, then `debian/` |
| tag `debian/0.17-19` | the Debian package as released |
| `fixes` | this fork's changes, on top of `debian` |

How the history was reconstructed, and the checksums of the source
files, are in [PROVENANCE.md](PROVENANCE.md).

## Things to know before using it

* The talk protocol has no authentication or encryption. See
  SECURITY.md. Do not expose `talkd` to networks you do not trust.
* The GitHub Actions workflow in `.github/workflows` has not been run
  yet; its commands were run locally except the CMake step.

## Licence

Berkeley-style licence with the original four clauses, including the
advertising clause, as it appears in the header of each source file and
in `debian/copyright`. This fork leaves those notices unchanged.

## Credits

Copyright (c) 1983 The Regents of the University of California, for the
original BSD `talk`. The Linux port is Netkit's. The Debian package was
split from `netstd` (created by Peter Tobias) by Herbert Xu, who also
fixed the old-talk emulation, per the ChangeLog. The quilt conversion
and CMake build are by Christoph Biedl, per the patch headers; later
uploads are by the Debian QA group.
