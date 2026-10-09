# READ THIS BEFORE YOU TOUCH ANYTHING UNDER vendor/

**Every file under `vendor/` is somebody else's work, frozen on purpose.**
Written 2026-09-20, at the author's asking.

The author, in his own words:

> *"as long as we don't change the code inside of /vendor, then we are clear to
> install the next copy that comes out for that software"*

> *"when we change the code inside of vendor, it has to be marked down
> somewhere"*

> *"so you don't accidentally make an edit to anything and it not be recorded"*

---

## The rule, in one sentence

**If you change a file that came from upstream, you write it down in
`vendor/edit_journal/<project>/EDITS.md` in the same sitting — before the
change is committed, not after.**

## Why the rule is worth keeping

An **unedited** project can be replaced with the next upstream release by
swapping one tarball. Nothing to re-apply, nothing to re-test beyond the build
itself. That is the whole reason this folder is arranged the way it is.

An **edited** project cannot. Every edit has to be found again, understood
again, re-applied to source that has moved under it, and re-tested — by
somebody who may not be you, years from now, with no memory of why the edit was
there. An edit that was never written down is worse still: it is discovered by
the upgrade breaking, and then it is archaeology.

So the journal is not bookkeeping. **It is the thing that decides whether the
next upgrade costs five minutes or a week**, and an empty `EDITS.md` is the
receipt that says five minutes.

## An empty EDITS.md is a statement, not an oversight

Every project folder gets an `EDITS.md` the day it is unpacked, **even when
there is nothing to record**. A file saying "no edits" is a positive claim that
someone checked. A missing file means nobody knows. Those are not the same, and
only one of them is safe to upgrade from.

---

## What counts as an edit

**These ARE edits. Journal them.**

- Changing any line of upstream's source, headers, or build files.
- Deleting an upstream file. *(This is the easy one to miss — the build that
  preceded this folder deleted two of GTK's own `.wrap` files and never said so
  anywhere but in a shell script's comments.)*
- Overwriting an upstream file with our own version of it.
- Applying a patch from anywhere, including a distribution's or Meson's
  wrapdb's. **A wrapdb "patch zip" is a third-party `meson.build` dropped inside
  upstream's source directory** — measured 2026-09-20, six of them were in use.
  That is an edit, and it is an edit somebody else maintains.
- Repairing generated build output after moving a tree. Harmless, throwaway,
  still an edit — see `edit_journal/gtk-old/EDITS.md` for the worked example.

**These are NOT edits. Do not journal them.**

- Passing options on the command line. `-Dfoo=disabled` changes nothing on disk.
- Adding *our own* files **beside** upstream's, in directories upstream does not
  own — build scripts, `.pc` files we write, staging prefixes.
- Build output, in a build directory outside the source tree.
- Unpacking a tarball. The tarball in `vendor/new/` stays sealed; what comes out
  of it is a copy.

**Prefer an option to an edit, always.** The whole build is arranged bottom-up —
each project built on its own into a shared `vendor/stage/` prefix — precisely
so that resolution is controlled by what pkg-config can see rather than by
reaching into anybody's source tree. If you find yourself about to edit
upstream, look first for the option that does the same thing.

---

## The layout

    vendor/
      README_FIRST.md      this file
      new/                 THE FROZEN SOURCE. 31 tarballs, sealed, plus SHA256SUMS.
      <project>/           one folder per project, unpacked from new/
      stage/               the shared install prefix everything is built into
      edit_journal/
        <project>/EDITS.md one per project folder, created the day it is unpacked

`vendor/new/` is the source of truth and **nothing in it is ever unpacked in
place**. It is committed to git — that is what `.gitignore`'s `!/vendor/new/*`
line is for, and its comment says why.

### vendor/new/SHA256SUMS, and what it does and does not prove

It records a hash for all 31 files, in three tiers, and **says which tier each
file is in**:

- **Upstream-verified (9)** — compared against a checksum upstream publishes
  beside the tarball, and matched. gtk, pango, graphene, glib, gdk-pixbuf,
  cairo, pixman, meson, vte.
- **GPG-signed upstream, signature not yet checked (6)** — upstream publishes a
  `.sig`/`.asc` but no plain checksum. freetype, libtiff, libjpeg-turbo, expat,
  pcre2, gperf.
- **Nothing published (16)** — the hash is from our own download. The four VTE
  needs (lz4, fmt, simdutf, fast_float, 2026-09-22) are GitHub tag archives and a
  release asset with no hash beside them; VTE's own `.wrap` files pin the same
  versions by git tag, which is a pin and not a checksum.

A hash in the last two tiers **pins the file against silent change from here
on. It does not prove the first download was authentic.** Say so, rather than
letting `sha256sum -c` passing read as more than it is.

---

## An edit is committed as a PATCH, not as a changed file

**The unpacked trees are not in git.** The tarballs are, so every project folder
is reproducible with one `tar xf`, and committing both would store 590 MB of
other people's source twice.

That has a consequence worth being explicit about: **a change made directly to
an unpacked tree is in no repository at all**, and the next person who
re-unpacks that tarball destroys it without a single warning. The journal entry
would survive and the change would not, which is the worst of both.

So an edit lands as two things, together:

    vendor/edit_journal/<project>/EDITS.md        the entry saying what and why
    vendor/edit_journal/<project>/NNN-<name>.patch   the change itself

    # made with, from inside vendor/<project>/:
    diff -u <file>.orig <file> > ../edit_journal/<project>/001-thing.patch

The tree stays reproducible, the change stays reviewable, and re-applying it
after an upgrade is a command instead of an act of memory. Number the patches so
their order is not a guess.

## What to write in an EDITS.md

One entry per edit. Date it, name the exact file and lines, and say **why** —
the why is the part that is impossible to reconstruct later:

    ## 2026-09-20 -- <file>, <what changed>

    **What:** the exact change, specific enough to re-apply by hand.
    **Why:** the failure it fixes. Quote the error if there was one.
    **Upstream:** reported / not reported / not worth reporting, and why.
    **On upgrade:** does the next release need this again, or does it fix it?

That last line is the one a future upgrade actually reads.
