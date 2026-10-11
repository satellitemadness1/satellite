#!/usr/bin/env python3
"""Write one Markdown page per confirmed satl error, and an index, from the hunt's data.

    make_pages.py <out-folder> <journal.jsonl of the finishing workflow, or "-">

Reads hunt-data/salvage.json (the first run's findings and skeptic verdicts) and the
finishing workflow's journal (the batch verdicts and the dedupe groups). With "-" it
writes the first run's confirmed errors alone, one page each, for a dry run.
"""
import json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
TREE = '/home/madness/code/cxx/satellite'
out_dir, journal = sys.argv[1], sys.argv[2]

with open(os.path.join(HERE, 'salvage.json')) as f:
    salvage = json.load(f)

verdicts = {}
for v in salvage['verifies']:
    v = dict(v, id=f"{v['area']}#{v['index']}")
    verdicts[v['id']] = v

groups = None
if journal != '-':
    with open(journal) as f:
        lines = [json.loads(l) for l in f if l.strip()]
    label = {j['agentId']: j.get('label', '') for j in lines if j.get('type') == 'started'}
    for j in lines:
        if j.get('type') != 'result':
            continue
        r = j['result']
        if isinstance(r, str):
            r = json.loads(r)
        l = label.get(j.get('agentId'), '')
        if l.startswith('verify:') and isinstance(r, dict):
            for v in r.get('verdicts', []):
                v['area'] = v['id'].split('#')[0]
                verdicts[v['id']] = v
        elif l == 'dedupe' and isinstance(r, dict):
            groups = r['groups']

groups_from_dedupe = groups is not None
if groups is None:
    confirmed = [v for v in verdicts.values() if v['verdict'] == 'confirmed-new']
    groups = [{'number': n + 1, 'slug': re.sub(r'[^a-z0-9]+', '-', v['title'].lower())[:50].strip('-'),
               'title': v['title'], 'members': [v['id']], 'severity': v['severity'], 'kind': v['kind'],
               'merge_note': ''} for n, v in enumerate(confirmed)]

SCRATCH = re.compile(r'/tmp/claude-1000/[^\s:"\']*/')
BANNER = re.compile(r'^(THE SATELLITE PROGRAMMING LANGUAGE|VERSION \d+ REVISION|CLANG\+\+|G\+\+ )')


def clean(text):
    """Scratch paths down to the file name; the start-up banner and the runner's [exit N] out."""
    text = SCRATCH.sub('', text or '')
    text = re.sub(r'SATL_PROBE_HOME=\S+ ', '', text)
    text = re.sub(r'\S*satlrun(_nocap)?\.sh', 'satl', text)
    out, skipping = [], True
    for line in text.splitlines():
        if skipping and (BANNER.match(line) or line.strip() in ('', '-' * 63)):
            continue
        skipping = False
        if re.fullmatch(r'\[exit \d+\]', line.strip()):
            continue
        out.append(line.rstrip())
    return '\n'.join(out).strip('\n')


def fence(text, lang=''):
    ticks = '```'
    while ticks in text:
        ticks += '`'
    return f'{ticks}{lang}\n{text}\n{ticks}'


def area_name(a):
    return a.replace('-2', '').replace('-', ' ')


provisional = not groups_from_dedupe

# The pages this script wrote last time are listed in .pages-manifest; only those are
# replaced, so nothing else in the folder is ever touched.
os.makedirs(out_dir, exist_ok=True)
manifest = os.path.join(out_dir, '.pages-manifest')
if os.path.exists(manifest):
    with open(manifest) as f:
        for old in f.read().split():
            if re.fullmatch(r'\d{3}-[a-z0-9-]+\.md|README\.md', old) and os.path.exists(os.path.join(out_dir, old)):
                os.remove(os.path.join(out_dir, old))
pages = []
for g in sorted(groups, key=lambda g: g['number']):
    members = [verdicts[m] for m in g['members'] if m in verdicts]
    if not members:
        continue
    v = members[0]
    number = f"{g['number']:03d}"
    slug = re.sub(r'[^a-z0-9-]+', '-', g['slug'].lower()).strip('-')[:60]
    name = f'{number}-{slug}.md'
    areas = sorted({area_name(m['area']) for m in members})
    setup = (v.get('setup') or '').strip()
    body = [
        f"# {number} -- {g['title']}",
        '',
        f"**Found:** 2026-09-26, satl 004 revision 08 build 0099 ({TREE}/build/satl), by running ordinary programs against it.  ",
        f"**Area:** {', '.join(areas)}  ",
        f"**Kind:** {g.get('kind') or v['kind']}  ",
        f"**Severity:** {g['severity']}",
        '',
        '## What happens',
        '',
        v['title'].rstrip('.') + '.',
        '',
        '## Program',
        '',
        fence(v['minimal_program'].rstrip('\n'), 'satellite'),
        '',
        'Run it with `satl program.satl`' + (f'. Setup: {clean(setup)}' if setup and setup.lower() not in ('none', 'none.', '') else '.'),
        '',
        '## What satl does',
        '',
        fence(clean(v['observed_output']) + f"\n\nexit {v['exit_code']}"),
        '',
        '## What it should do',
        '',
        clean(v['expected']),
        '',
        clean(v['why_expected']),
        '',
        '## Variants',
        '',
        clean(v['variants']) or 'None recorded.',
        '',
        '## Where it comes from',
        '',
        (clean(v['source_location']) or 'not located') + ('' if not v.get('cause') else '\n\n' + clean(v['cause'])),
        '',
        '## Notes',
        '',
        '- Why the skeptic kept it: ' + clean(v['reason']),
        '- Nearest known entry: ' + (clean(v.get('known_ref')) or 'none'),
    ]
    if g.get('merge_note'):
        body.append('- Merged: ' + clean(g['merge_note']))
    for m in members[1:]:
        body += ['', f"### Also seen as: {m['title']}", '', fence(m['minimal_program'].rstrip('\n'), 'satellite'),
                 '', fence(clean(m['observed_output']) + f"\n\nexit {m['exit_code']}")]
    with open(os.path.join(out_dir, name), 'w') as f:
        f.write('\n'.join(body) + '\n')
    pages.append((number, name, g['title'], g['severity'], g.get('kind') or v['kind'], ', '.join(areas)))

counts = {}
for v in verdicts.values():
    counts[v['verdict']] = counts.get(v['verdict'], 0) + 1
index = [
    '# Errors found in satellite 004 (satl build 0099)',
    '',
    'One page per error. Found on 2026-09-26 by running thousands of ordinary satellite programs',
    'against satl (revision 08, build 0099) and re-checking each suspected error: a second agent',
    're-ran it, shrank it to the smallest program that still shows it, and checked it against',
    '`ERROR.md`, `errors.md`, `SCRATCH.md/ERRORS2.md`, `NEW_ERROR_LIST.md`, `CONTAINERS.md`,',
    '`DESIGN.md` and the help topic. What was already known or is by design is left out.',
    '',
    f"Verdicts on the {sum(counts.values())} suspected errors: " + ', '.join(f'{n} {k}' for k, n in sorted(counts.items(), key=lambda x: -x[1])) + '.',
    (f'**{len(pages)} pages**, one per confirmed error, duplicates not merged yet.' if provisional
     else f'After merging the ones that are one error: **{len(pages)} pages**.'),
    '',
    'The runner, the raw verdicts and this script are in `hunt-data/` beside this page. The hunters\'',
    'own programs (2,258 of them, 460 MB with their bytecode) stayed on the author\'s machine at',
    '`/home/madness/Documents/satl/hunt-data/probes/`; each page carries its smallest program.',
    '',
    '**NOT YET RE-CHECKED.** Every page was true of build 0099. What landed after it (0da03d9,',
    '5b8cdf3, 15f5119) and the two branches not yet merged (`m16-string-methods`,',
    '`errors-2026-09-26`) may fix some; run each page\'s program on today\'s satl before fixing it.',
    '',
] + ([
    '> **PROVISIONAL.** Written before every check finished and before duplicates were merged,',
    '> so two pages can describe one error. Re-run `hunt-data/make_pages.py` on the finished',
    '> journal to replace these pages with the merged set.',
    '',
] if provisional else []) + [
    '| # | error | severity | kind | area |',
    '|---|---|---|---|---|',
]
for number, name, title, sev, kind, areas in pages:
    title = title.replace('|', '\\|')
    index.append(f'| {number} | [{title}]({name}) | {sev} | {kind} | {areas} |')
with open(os.path.join(out_dir, 'README.md'), 'w') as f:
    f.write('\n'.join(index) + '\n')
with open(manifest, 'w') as f:
    f.write('\n'.join([p[1] for p in pages] + ['README.md']) + '\n')
print(len(pages), 'pages written to', out_dir)
