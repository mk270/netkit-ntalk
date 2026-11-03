#!/usr/bin/env python3
"""Drive a talkd binary over loopback UDP.

usage: probe.py TALKD SCENARIO...   (scenarios: expire, cap)

talkd expects to be started by inetd: UDP socket on fds 0 and 1.
"""
import os
import socket
import struct
import subprocess
import sys
import time

LEAVE_INVITE, LOOK_UP, DELETE, ANNOUNCE = 0, 1, 2, 3
SUCCESS, FAILED = 0, 2


def ctl(type_, l_name, r_name, tty=b"", id_num=0, pid=1, port=40000):
    """Build a 84-byte CTL_MSG (see include/prot_talkd.h)."""
    ta = struct.pack("!HH4s8s", 2, port, socket.inet_aton("127.0.0.1"), b"")
    return (struct.pack("!BBBBI", 1, type_, 0, 0, id_num) + ta + ta
            + struct.pack("!I", pid) + l_name.ljust(12, b"\0")
            + r_name.ljust(12, b"\0") + tty.ljust(16, b"\0"))


class Daemon:
    def __init__(self, path):
        self.srv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.srv.bind(("127.0.0.1", 0))
        self.port = self.srv.getsockname()[1]
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
        self.proc = subprocess.Popen(
            [path, "-q"], stdin=self.srv.fileno(),
            stdout=self.srv.fileno(), stderr=subprocess.PIPE, env=env,
            pass_fds=[self.srv.fileno()])
        self.cli = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.cli.bind(("127.0.0.1", 0))
        self.cli.settimeout(5)

    def ask(self, pkt):
        """Send one request; return (type, answer, id) or None."""
        self.cli.sendto(pkt, ("127.0.0.1", self.port))
        try:
            data = self.cli.recvfrom(256)[0]
        except socket.timeout:
            return None
        _, type_, answer, _, id_num = struct.unpack("!BBBBI", data[:8])
        return type_, answer, id_num

    def alive(self):
        time.sleep(0.2)
        return self.proc.poll() is None

    def stderr(self):
        self.proc.kill()
        return self.proc.stderr.read().decode(errors="replace")


def expire(path):
    """An aged-out invitation must not crash the daemon."""
    d = Daemon(path)
    assert d.ask(ctl(LEAVE_INVITE, b"alice", b"bob"))[1] == SUCCESS
    assert d.ask(ctl(LOOK_UP, b"bob", b"alice"))[1] == SUCCESS
    time.sleep(int(os.environ.get("TALKD_MAX_LIFE", "2")) + 1)
    r = d.ask(ctl(LOOK_UP, b"bob", b"alice"))
    assert r is not None and d.alive(), d.stderr()[:2000]
    # it expired, so the lookup must now fail with NOT_HERE
    assert r[1] == 1, "expired invitation still found: %r" % (r,)


def cap(path):
    """The table must refuse invitations beyond its limit."""
    limit = int(os.environ["TALKD_MAX_TABLE"])
    d = Daemon(path)
    answers = [d.ask(ctl(LEAVE_INVITE, b"u%d" % i, b"v%d" % i,
                         pid=i + 1))[1] for i in range(limit + 5)]
    assert answers[:limit] == [SUCCESS] * limit, answers
    assert FAILED in answers[limit:], answers
    assert d.alive(), d.stderr()[:2000]


if __name__ == "__main__":
    bad = 0
    for name in sys.argv[2:]:
        try:
            globals()[name](sys.argv[1])
            print("PASS", name)
        except AssertionError as e:
            print("FAIL", name, e)
            bad = 1
    sys.exit(bad)
