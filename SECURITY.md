# Security

## Reporting a problem

**TODO (maintainer to fill in before publishing):** say here how to
report a vulnerability privately, for example GitHub's "Report a
vulnerability" feature once it is enabled on the repository, or an
email address.

## Read this before running `talkd`

`talkd` listens on UDP (the ntalk service port, 518) and normally runs
as root, because it writes to other users' terminals. The protocol it
speaks has no authentication or encryption:

* The caller's name in a request is whatever the sending client wrote.
  Anyone who can send a UDP packet to `talkd` can announce a call to a
  logged-in user claiming to be anyone.
* There is no rate limit on announcements.
* Conversations are plain text on the network, and the first program
  to connect to the caller's rendezvous port is taken to be the other
  party.

This comes from the protocol, not from a bug a patch can fix. Do not
expose `talkd` to networks you do not trust: block UDP 518 at the
firewall, and use libwrap (`hosts.allow`/`hosts.deny`) if your build
has it. The check that requests come from a source port of 1024 or
above is not a security measure.

## Fixed in this fork

* **Use-after-free in `expire()` (remote crash).** `table.c` freed an
  expired invitation and then read it to find the next one. Once any
  invitation was older than `MAX_LIFE` (60 s), the next lookup,
  announcement or invite request from anyone crashed the daemon.
  Reproduced under AddressSanitizer: leave an invitation, wait, send
  such a request. A local user can leave an invitation directly. A
  remote host cannot (invite requests are only accepted from the local
  machine), but an announcement to a logged-in user who accepts
  messages also creates an entry, so a remote attacker who can name
  such a user can trigger it. Under inetd the daemon is restarted on
  the next packet; run standalone it stays down.
* **Uninitialised memory sent in replies.** For a new invitation the
  reply's answer, padding and address fields were never set, so stack
  bytes went out in the UDP reply. Found with MemorySanitizer; replies
  are now zeroed first.
* **Unbounded invitation table.** Every request could allocate an
  entry that lived 60 s. The table is now capped at 1024 entries;
  further requests get `FAILED`. This bounds memory, not abuse.
* **Off-by-one in debug logging** (`-d`): a request with type 4
  indexed one past the name table.
* **tty names from utmp.** A tty name longer than the 16-byte field
  could be copied past it, and `ut_line` was assumed to be terminated.
  Such names are now skipped.
* **Announcing to the wrong file.** `talkd` opens the callee's tty as
  root. It now opens it non-blocking (so a modem line or FIFO cannot
  hang the daemon) and checks that it is a terminal owned by the person
  being called.

The last two have no automated test, because they need utmp and
pseudo-terminal fixtures. The others are covered by `tests/run.sh`.

## Reviewed and found sound

* The announcement banner replaces every non-printable byte, so a
  caller cannot inject escape sequences into the callee's terminal.
* The `talk` client shows control characters from the peer as `^X`
  rather than passing them to your terminal, and its scrollback is
  bounded.
* Names in requests are terminated before use.

## Known limitations, not fixed

* The identity limits above, and IPv4 only.
* In the `talk` client, the first connection to the caller's port is
  accepted without checking who it is from.
* The client trusts the address a daemon returns for the call and
  connects to it.
* Replies from a daemon are matched on version and type only, with no
  per-request token.
* With `-d` or `-p`, request fields are logged as received, so a
  caller can put newlines in the log.
* libwrap itself is unmaintained; see the build options in the README.
* The `talk` client has had no review beyond reading the main paths,
  and has no tests.
