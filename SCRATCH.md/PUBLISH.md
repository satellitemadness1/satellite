# PUBLISH THE WEBSITE — DONE 2026-09-26

**Published 2026-09-26 ~06:10 UTC** (the server answered again), at his word ("let's publish
the site"): every page and post as below, plus four new posts -- recursion-a-million-deep (548),
a-syntax-tree-of-objects (549), build-an-interpreter-in-cpp (550), and containers-of-any-shape
(547, the day's map literal, nested containers and satellite.access). The maps post, the
history-and-access post, the full reference's containers/map/access sections and one tutorial
sentence were rewritten first: they said map and satellite.access were refused. 327 examples,
0 failing. Nothing on the site had changed since his 2026-09-24 21:13-21:40 edits; his
categories and tags were pulled first and survived. Then tags were added to the 19 posts that
had none (his "you can add more tags"), in his style, by one whole-site upload.py run without
the pull (the pull would have put the site's empty tags back). publish.sh now lists
containers-of-any-shape after multiple.

**Next time:** after M16 merges, post-strings says which methods are refused -- check_examples
will flag those examples as FLIPPED; rewrite them and publish.sh again.

---

(What follows is the note as it was written before publishing.)


The author, 2026-09-25, late: *"we will publish the website later tonight, after we have worked
for awhile... leave yourself a note in /SCRATCH.md/PUBLISH.md that we need to publish the
website"*.

**Nothing has been published since 2026-09-24.** Every change below exists only in the local
sources, `~/.config/satellite-foundation/site/`, which are not in git.

## What is waiting

- **Every page matches satl as it is tonight.** A string + a number joins; a number + text that
  spells a number adds (`4 + "2"` is 6); a bool joins as `true`/`false`; escapes; S103's
  wording (main's last line); recursion with no depth limit; tutorial exercise 10 (now a
  misspelled word).
- **Three new posts**, already in `publish.sh`'s list: `build-an-interpreter-in-cpp`,
  `recursion-a-million-deep` and `a-syntax-tree-of-objects`.
- **A 20px gap** between each code box and the output box straight after it
  (`tools/blocks.py`, his ask).
- **His categories and tags.** He added them by hand on WordPress (2026-09-24, 21:13–21:40 GMT)
  and they must survive. `publish.sh` now runs `tools/pull_taxonomy.py` first, which copies
  them into each page's front matter; `upload.py` sends them back. If the site can't be read,
  `publish.sh` stops and uploads nothing.
- **"You can add more tags"** (his words). Do it after the pull, once his tags are in the front
  matter to match.

## How, when he says go

1. **Check the server answers.** It stopped answering this machine at about 20:30 on
   2026-09-25:
   `curl -s -o /dev/null -w '%{http_code}\n' --max-time 20 https://satellite.foundation/`
   should print 200.
2. **Check every example still matches satl.** The satl build may have moved since:
   `/usr/bin/python3 ~/.config/satellite-foundation/tools/check_examples.py ~/.config/satellite-foundation/site/*.wp.md`
   should end "0 failing". If an output box is stale, run
   `/usr/bin/python3 ~/.config/satellite-foundation/tools/refresh_outputs.py ~/.config/satellite-foundation/site/*.wp.md ~/.config/satellite-foundation/site/parts/*.md`.
   It only rewrites boxes whose example still passes or fails as marked. A FLIPPED example
   needs its words rewritten by hand.
3. **Optionally, look first:**
   `/usr/bin/python3 ~/.config/satellite-foundation/tools/pull_taxonomy.py --dry-run`
   shows the categories and tags it would write.
4. **Publish by the FULL PATH:** `~/.config/satellite-foundation/publish.sh`. Running
   `./publish.sh` fails, because it builds its tool paths from `$0`.
5. **Afterwards:** open two or three pages (a post with an output box, the full reference) and
   check the gap, the tags, and that links between pages resolve.

## Never

- **Never upload a few pages with `upload.py` alone.** It re-dates those posts to the top of the
  blog, and turns their links to pages outside the upload into bare links to the site.
- **Never change a page this tool didn't make.** `made.json` lists the ones it made; LICENSES and
  PRIVACY POLICY are his.
- **Never publish while `check_examples.py` reports a failure.**
