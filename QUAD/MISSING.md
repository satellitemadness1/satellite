# What satellite is missing for QUAD

**Written 2026-09-12**, against the `satl` built from this tree that day. Every
row marked *probed* was checked by running a small program, and the error
code is what `satl` actually said. Re-probe before trusting a row once the
interpreter has moved.

**What QUAD is:** the AI project at `/home/madness/code/satl/quad`, written in
satellite. It reads Common Crawl and Wikipedia, builds a word dictionary by hand
and a word-connection index by program, and runs quad_infinity's
sky/flock/rack mind on top of them. Its plan is
`/home/madness/code/satl/quad/PLAN.md`, and the M-numbers in the "QUAD needs it
at" column are that plan's milestones.

**Numbering:** "SATELLITE/M27" means a FOREIGN_MILESTONES promise. "PLAN M25"
means this repo's PLAN.md. "none" means nothing anywhere has taken the
feature yet.

---

## 1. Blocking: QUAD cannot be written without these, or only badly

| # | missing | evidence | QUAD needs it at | workaround until then | milestone |
|---|---|---|---|---|---|
| 1 | **create a directory** | probed: `satellite.directory.new(d)` → **S0521** "`new` is not a word the language has under satellite.directory". The directory words are change/current/exists/list | M1 (`files-002/`, `files-003/`…), M2 (`dictionary/a_word/`…), M4 (`index/a_words/`…) | make the folders by hand before a run. This breaks for M1, which cannot know in advance how many folders it will fill | none |
| 2 | **ordering strings: `<` `>` on strings** | probed: `"apple" < "banana"` → **S0712** "string has no order" | M3/M4: merge-sorting files bigger than memory. The merge step has to compare two lines | put the two lines in a list and `sort()` it. Slow, but it works (`list.sort()` on strings is fine: 200,000 strings in 1.1 s) | none |
| 3 | **`log` and `exp`** | probed: no such word under `satellite.variable.number` or `float` (**S0723**) | M4/M5: surprise = log of strength ÷ (seen × seen), i.e. PMI; M9: altitude from the entropy of a word's neighbours | write a series `log` in satellite with `number` arithmetic. Unmeasured, and probably slow over millions of words | none |
| 4 | **math on `satellite.variable.float`** | probed: `f.power(x)`, `f.sqrt()` → **S0723**; `number.power(2.5)` with a float argument → **S0713** "wants a number and this one is float". The float's methods are to_string/string/number/binary/hex only | everywhere QUAD does arithmetic on strengths. The author wants strengths held as `float` | do the math in `satellite.variable.number`, which holds decimals and whose `power` accepts fractional exponents (example/draw.satl does this) | none |
| 5 | **running another program** | probed: `satellite.system.run("ls")` → **S0521** | M10: QUAD checks its own proposed change with `satl --check` and runs the tests. This is the self-improvement loop | none inside the language. It is also the one gap QUAD must not paper over with a shell script | none |
| 6 | **including another `.satl` file** | `satellite.include(spaceship)` is `1 1 2`; MILESTONES/README.md shows PLAN M25 has not landed | every milestone. The WARC reader, the tokenizer and the sky are shared by several programs | copy capsules between programs. They drift apart | PLAN M25 |
| 7 | **locks / atomic updates** | M23 threads landed, and its note says read-add-write on a shared `satellite.library` value is three operations | M1.6: 24 threads writing shards without two taking the same shard number. M3/M4: parallel counting | give each thread its own output range, so nothing is shared | SATELLITE/M40 |

### 1.1 Blocking for the data sources QUAD plans to read

Found 2026-09-12 by fetching the headers of the actual download files. Satellite
decompresses **gzip only** (`satellite_file/gzip.hpp`).

| # | missing | the file that needs it | size | milestone |
|---|---|---|---|---|
| 1a | **bzip2 decompression** | `enwiki-latest-pages-articles-multistream.xml.bz2` (English Wikipedia); also `enwiktionary-…bz2` and `simplewiki-…bz2` | 26.8 GB / 1.6 GB / 356 MB | none |
| 1b | **zstd decompression** | RedPajama-1T `common_crawl/*/…jsonl.zst` (859 files, ~2.2 GB each) | ~1.9 TB (estimate) | none |
| 1c | **decoding a JSON string** (`\n`, `\"`, `\uXXXX`) | every RedPajama line is `{"text": "…", "meta": {…}}` | — | none. It can be written in satellite; a builtin would be much faster |
| 1d | reading a `.tar` | `WordNet-3.0.tar.gz` (the gzip layer already works) | 11 MB | none. Unpacking once by hand is harmless |

Readable **today**, because they are uncompressed `.jsonl`: RedPajama's
`wikipedia/wiki.jsonl` (120 GB, 20 languages, 2023), `stackexchange.jsonl`
(80 GB), `c4/*.jsonl` and `github/*.jsonl`. They still need 1c.

## 2. Friction: everything still works, but every program pays for it

| # | missing | evidence | cost in QUAD | milestone |
|---|---|---|---|---|
| 8 | `break` / `continue` | probed: **S0212** "satellite.statement has if, else, while and for" | every record-skipping loop in the WARC reader; quad_infinity has 66 sites | SATELLITE/M27 |
| 9 | `&&` / `\|\|` | probed: **S0201** at the `&` / `\|` | "HTTP 200 **and** text/html **and** English" becomes three nested ifs | SATELLITE/M28 |
| 10 | method chaining past one hop, **including on a literal** | probed: `s.trim().lower()` → **S0720**; `"a b".split(" ")` also **S0720** | a named temporary for every step of tokenizing | SATELLITE/M29 |
| 11 | range `for` over a list | probed: `for (satellite.variable.string x : l)` → **S0201** | index loops everywhere; quad_infinity has 142 | none |
| 12 | `+=` | probed: `i += 1` → **S0231** | counters are `count = count + 1` | none |
| 13 | weak references (`.pointer()`) | MILESTONES/M26.md: "`.pointer()` is still owed" | the sky is a graph with links in both directions. As spacesuit references, it leaks by refcount cycle | QUAD stores link targets as node numbers, not references (M26) |

## 3. Missing, and not needed yet

| # | missing | evidence | would be used for |
|---|---|---|---|
| 14 | seek / read at an offset | CONTINUING.md §6.3 "not in the word tree"; only `read_line` / `read_all` | jumping into a 64 MiB shard or a large index file. QUAD's one-file-per-word layout avoids it |
| 15 | rename / move a file | not in `satl --words` | write-then-rename, so a killed run never leaves a half-written `progress.txt` |
| 16 | a file's size without reading it | not in `satl --words` for `file` | checking shard sizes after a run (the writer counts its own bytes, so M1 does not need it) |
| 17 | stderr | CONTINUING.md §6.3 | keeping progress messages out of piped output |
| 18 | a monotonic clock | CONTINUING.md §6.3 | measuring throughput (pages/s) without wall-clock jumps |
| 19 | character classes / character codes | the string's methods have no `is_letter` or code; `string.at(n)` answers a string | the tokenizer. The workaround, `"abc…z".contains(c)`, is **unmeasured** at crawl scale |
| 20 | colour and erase-to-end-of-line | PLAN M30 is being built now (`display(x, end="")` landed in commit ea60efc) | quad_infinity's live display of the sky and flock |
| 21 | writing `.gz` | `satellite_file/gzip.hpp`: "READING ONLY, TODAY" | compressing index shards. Not planned |

## 4. No longer missing: corrections to older notes

`/home/madness/code/satl/quad/need.txt` listed several of these as gaps. As
of today they are not:

| was listed as missing | now | how checked |
|---|---|---|
| `map.set` copies the whole map, O(n) per write | **fixed**: 2,000 sets 0.011 s, 20,000 sets 0.049 s, so linear | probed |
| spacesuits (38 structs + 11 classes in quad_infinity) | **landed**, M26. A `list<node>` of spacesuits appends, indexes, and shares by reference | probed |
| a seeded fractional random draw | **landed**, M21: `satellite.random.seeded(seed)` and `seeded(min, max, step)` replay the same draws | probed: `seeded(42)` then two draws |
| threads | **landed**, M23 | MILESTONES/README.md |
| reading `.warc.gz` | **works**. Multi-member gzip is read through; 2M lines of a Common Crawl WARC at ~20 MB/s, and bytes round-trip exactly | probed |
| fractional powers | **work**, on `number` (not `float`, see row 4) | example/draw.satl |

---

## 5. If only three get built

1. **Create a directory (row 1).** It is small, and M1 cannot run unattended
   without it.
2. **String ordering (row 2).** Every sort bigger than memory needs it, and
   "a mind that reads more than it can hold has to sort" (need.txt §3).
3. **`log` plus float math (rows 3–4).** Every strength QUAD compares goes
   through them.

Row 5, running a program, is the most important one long-term, because it is
what lets QUAD work on itself. But nothing needs it before QUAD's M10.
