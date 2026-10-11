# EDITS -- vendor/fast_float/

**Project:** fast_float-8.1.0
**From:** `vendor/new/fast_float-8.1.0.tar.gz` -- unpacked to `vendor/fast_float/fast_float-8.1.0/` on 2026-09-22
**Hash:** recorded in `vendor/new/SHA256SUMS` (last tier: nothing published upstream to compare
against -- the hash pins the file from here on and proves nothing about the first download)
**Why it is here:** VTE 0.84.1 needs it (vendor/build_stack_recipes.py says exactly what for).
The version is the one VTE's own `subprojects/*.wrap` pins; VTE's bundled copy of it is not used.

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
where that line falls.
