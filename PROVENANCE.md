# Provenance

Where the code came from, and how to check it.

## Source files

| File | SHA-256 |
| --- | --- |
| `netkit-ntalk_0.17.orig.tar.gz` | `d8f614e8d4d7981a0adf0f84d2d3954018d5fe0e57a8e5731256d66cbcf45e90` |
| `netkit-ntalk_0.17-19.debian.tar.xz` | `9052b945a74f74b8c78f8fa1f22cc0b6071a6f31f1ccd609ef5b6b9550f0b8cd` |

These match the checksums in Debian's `netkit-ntalk_0.17-19.dsc`. The
original tarball was downloaded from snapshot.debian.org:

* `https://snapshot.debian.org/archive/debian/20050312T000000Z/pool/main/n/netkit-ntalk/netkit-ntalk_0.17.orig.tar.gz`

The exact download location of the `.dsc` and `.debian.tar.xz` was not
recorded; the checksums above are what tie them to the Debian archive.
The `.dsc` carries a PGP signature, which was not verified.

## How the history was built

The package is in the `3.0 (quilt)` format: an upstream tarball plus
patches. The git history was generated from it:

1. `master`: the `.orig.tar.gz` extracted as one commit, tagged
   `upstream/0.17`.
2. `debian`: each patch in `debian/patches/series` applied in order
   with `patch -p1`, one commit per patch. The commit subject comes
   from the patch's `Description:` header and the body names the patch
   file and its `Author:`.
3. A final commit adding `debian/` (without `patches/`), tagged
   `debian/0.17-19`.

Note that the first Debian patch, `old-debian-patches.patch`, is
itself a squash: its header says it collects "old patches found when
converting to quilt patch queue". The many separate changes it contains
(libwrap, SunOS compatibility, `-q`, and others) therefore appear as a
single commit here.

Afterwards the `debian/0.17-19` tree was compared with a fresh
`dpkg-source -x netkit-ntalk_0.17-19.dsc`; the two are identical apart
from quilt's `.pc/` directory and `debian/patches/` (see Reproducing).

## What the history does not tell you

* The commits up to `debian/0.17-19` were created by a script. Their
  author is the placeholder `re-talk import <import@localhost>` and
  their dates are the day the import was done, **not** the original
  authors or dates.
* Upstream's own history (before 0.17), and the individual older Debian
  patches inside the squash above, are not included.
* `wordwrap.patch` in the tree is upstream's own, deliberately unapplied
  patch (its header explains why); it is not part of the build.

## Reproducing

    dpkg-source -x netkit-ntalk_0.17-19.dsc extracted
    mkdir pristine
    git archive debian/0.17-19 | tar -x -C pristine
    diff -r extracted pristine

Expect exactly two differences: quilt's `.pc/` directory and
`debian/patches/`, both present in `extracted` only. The patches are
already applied in the git tree, as commits.
