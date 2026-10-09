import json, sys, subprocess, re
from concurrent.futures import ThreadPoolExecutor
S=sys.argv[1]
with open(S+'/salvage.json') as f: d=json.load(f)
verified={(v['area'],v['index']) for v in d['verifies']}
jobs=[(a,i,x) for a,r in d['hunts'].items() for i,x in enumerate(r['findings']) if (a,i) not in verified]
def run(job):
    a,i,x=job
    try:
        p=subprocess.run(['bash','-c',x['command']], capture_output=True, text=True, timeout=150, errors='replace')
        out=p.stdout+p.stderr
        m=re.findall(r'\[exit (\d+)\]', out); code=int(m[-1]) if m else p.returncode
    except subprocess.TimeoutExpired:
        out='(re-run passed 150 s)'; code=-1
    same_code = code==x['exit_code']
    # a distinctive line of what the hunter saw, found again?
    lines=[l.strip() for l in x['observed'].splitlines() if len(l.strip())>12 and not l.strip().startswith(('[exit','---','THE SATELLITE','VERSION','CLANG'))]
    hits=sum(1 for l in lines[:6] if l[:60] in out)
    return {'area':a,'index':i,'title':x['title'],'reported_exit':x['exit_code'],'rerun_exit':code,'same_exit':same_code,'observed_lines_found':f'{hits}/{min(6,len(lines))}','rerun_output':out[-6000:]}
with ThreadPoolExecutor(8) as ex: res=list(ex.map(run, jobs))
with open(S+'/rerun.json','w') as f: json.dump(res,f,indent=1)
ok=sum(r['same_exit'] for r in res)
print(len(res),'re-run;',ok,'gave the reported exit code')
for r in res:
    if not r['same_exit']: print('  DIFFERS', r['area'], r['index'], r['reported_exit'],'->',r['rerun_exit'],'|',r['title'][:90])
