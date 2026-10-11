# EDITS -- vendor/harfbuzz/

**Project:** harfbuzz-14.4.0
**From:** `vendor/new/harfbuzz-14.4.0.tar.xz` -- unpacked to `vendor/harfbuzz/harfbuzz-14.4.0/` on 2026-09-20
**Hash:** recorded in `vendor/new/SHA256SUMS` (which tier, and what that proves,
is stated there)

---

## No edits.

Upstream's source is **exactly as shipped**. Nothing in this tree has been
changed, deleted, patched or overwritten.

**What that buys:** this project can take its next upstream release by replacing
the tarball in `vendor/new/`, re-unpacking, and building. There is nothing to
re-apply and nothing to re-discover. That is the entire point of the rule, and
this file is the receipt.

**If you change anything here**, add an entry below in the same sitting -- not
afterwards -- in the form `vendor/README_FIRST.md` sets out:

    ## <date> -- <file>, <what changed>

    **What:** the exact change, specific enough to re-apply by hand.
    **Why:** the failure it fixes. Quote the error if there was one.
    **Upstream:** reported / not reported / not worth reporting, and why.
    **On upgrade:** does the next release need this again, or does it fix it?

Options passed on the command line are **not** edits, and neither are our own
files placed beside upstream's, nor build output. Read `README_FIRST.md` for
where that line falls -- it is not obvious, and a Meson wrapdb "patch" is on the
wrong side of it.
