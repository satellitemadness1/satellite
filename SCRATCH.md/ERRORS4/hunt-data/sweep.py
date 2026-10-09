import sys, subprocess, re, glob, os
from concurrent.futures import ThreadPoolExecutor
S=sys.argv[1]
progs=sorted(glob.glob(S+'/work/**/*.satl', recursive=True))
def run(p):
    area=p.split('/work/')[1].split('/')[0]
    env=dict(os.environ, SATL_PROBE_HOME=f'{S}/sweep-home/{area}', SATL_TIMEOUT='20')
    try:
        r=subprocess.run([S+'/satlrun.sh', p], capture_output=True, text=True, timeout=60, errors='replace', env=env, cwd=os.path.dirname(p))
        out=r.stdout+r.stderr
        m=re.findall(r'\[exit (\d+)\]', out); code=int(m[-1]) if m else r.returncode
    except subprocess.TimeoutExpired:
        out=''; code=-1
    bad = code in (-1,124,137) or code>=128 or re.search(r'terminate called|std::|Segmentation|Aborted|core dumped|AddressSanitizer|what\(\)', out)
    return (p, code, out[-1500:]) if bad else None
with ThreadPoolExecutor(10) as ex: res=[x for x in ex.map(run, progs) if x]
print(len(progs),'programs;',len(res),'crashed / hung / leaked')
with open(S+'/sweep.txt','w') as f:
    for p,c,o in res: f.write(f'=== {c} {p}\n{o}\n')
for p,c,o in res: print(c, p.split('/work/')[1])
