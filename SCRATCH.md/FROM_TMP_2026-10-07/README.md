# Copied out of /tmp on 2026-10-11

/tmp is emptied of anything 10 days old, so these would have gone around 10-18.

- file_operations/: the 10-07 file-operations rewrite (FO-5, FO-8, FO-9, the race, help pages) as three
  patches, job_a/b/c, against the 10-07 tree, plus the brief and Phase 2 plan it was built from. Never landed;
  it needs rebasing onto today's tree. From session e99fe5f0.
- pgo/: the PGO patch (sudo's PATH loses clang 24, so clang 21 meets its profile) and the 045-optimise.mk it
  was made against. Drafted 10-07, never applied. From session f00b04df.
