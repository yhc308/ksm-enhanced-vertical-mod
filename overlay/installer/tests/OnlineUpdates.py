"""Exercise production WinHTTP and update selection against an owned loopback server."""
from pathlib import Path
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
import copy,json,subprocess,threading,time,uuid,argparse
parser=argparse.ArgumentParser();parser.add_argument('--legacy-binary',type=Path);args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[2];work=ROOT/'build/online-update-tests'/uuid.uuid4().hex;work.mkdir(parents=True)
repo='https://github.com/yhc308/ksm-enhanced-vertical-mod/'
current={'version':'0.5.4-beta4','buildId':'20261117-025923-43ae7184','installerRevisionNumber':1,'installerRevision':'20261120-installer-ui1'}
def metadata(**changes):
 m={'schemaVersion':1,'supportedGame':'2.2.0','runtimeContract':'ksm-2.2.0-enhanced-skin-v1','package':dict(current),'zipName':'KsmEnhancedVertical_0.5.4-beta4.zip','zipSha256':'a'*64};m.update(changes);return m

def release(tag='v0.5.4-beta4',**changes):
 r={'tag_name':tag,'html_url':repo+'releases/tag/'+tag,'draft':False,'prerelease':True,'assets':[{'name':'update-info-ksm-2.2.0.json','browser_download_url':repo+'releases/download/'+tag+'/update-info-ksm-2.2.0.json'},{'name':'KsmEnhancedVertical_0.5.4-beta4.zip'}]};r.update(changes);return r
cases={}
def add(name,expected,rels=None,meta=None,**settings):cases[name]={'expected':expected,'releases':rels if rels is not None else [release()],'metadata':meta if meta is not None else metadata(),**settings}
add('capital-beta','available',meta=metadata(package={**current,'version':'0.5.5-Beta5'}));add('case-equivalent','current',meta=metadata(package={**current,'version':'0.5.4-Beta4'}));add('same-capital-current','current',meta=metadata(package={**current,'version':'0.5.5-Beta5'}),currentOverride={**current,'version':'0.5.5-Beta5'});add('next-capital-current','available',meta=metadata(package={**current,'version':'0.5.6-Beta6'}),currentOverride={**current,'version':'0.5.5-Beta5'});add('legacy-wire-display','available',meta=metadata(package={**current,'version':'0.5.5-beta5','displayVersion':'0.5.5-Beta5'}));add('mismatched-display','unavailable',meta=metadata(package={**current,'displayVersion':'9.9.9'}));add('same','current');add('installer-revision','available',meta=metadata(package={**current,'installerRevisionNumber':2}));add('older-installer','current',meta=metadata(package={**current,'installerRevisionNumber':0}))
add('new-version','available',meta=metadata(package={**current,'version':'0.5.5-beta5'}));add('stable-version','available',meta=metadata(package={**current,'version':'0.5.4'}));add('numeric-beta','available',meta=metadata(package={**current,'version':'0.5.4-beta10'}))
add('new-build','available',meta=metadata(package={**current,'buildId':'20261120-010101-12345678','installerRevisionNumber':0}));add('old-build','current',meta=metadata(package={**current,'buildId':'20261110-010101-12345678'}))
add('different-game','unknown',meta=metadata(supportedGame='2.1.0'));add('different-contract','unknown',meta=metadata(runtimeContract='ksm-2.1.0-enhanced-skin-v2'));add('draft','unknown',rels=[release(draft=True)]);add('legacy-release','unknown',rels=[release(assets=[{'name':'old.zip'}])]);add('empty','unknown',rels=[])
add('invalid-json','unavailable',raw='{');add('invalid-schema','unavailable',meta=metadata(schemaVersion=9));add('bad-revision','unavailable',meta=metadata(package={**current,'installerRevisionNumber':-1}));add('missing-revision','unavailable',meta=metadata(package={k:v for k,v in current.items()if k!='installerRevisionNumber'}));add('invalid-version','unavailable',meta=metadata(package={**current,'version':'oops'}));add('invalid-build','unavailable',meta=metadata(package={**current,'buildId':'oops'}));add('invalid-hash','unavailable',meta=metadata(zipSha256='broken'));add('zip-traversal','unavailable',meta=metadata(zipName='../evil.zip'));add('zip-missing','unavailable',rels=[release(assets=release()['assets'][:1])])
r=release();r['assets'][0]['browser_download_url']='https://evil.example/update.json';add('untrusted-asset','unavailable',rels=[r]);add('untrusted-page','unknown',rels=[release(html_url='https://evil.example/')]);add('rate-limit','unavailable',http=403);add('not-found','unavailable',http=404);add('offline','unavailable',offline=True);add('timeout','unavailable',delay=2);add('cancel','cancelled',delay=2,cancel=True);add('untrusted-redirect','unavailable',redirect='https://evil.example/');add('oversized','unavailable',huge=True)
# Pagination and semantic ordering: an older release later in the response cannot replace a newer candidate.
add('pagination','current',rels=[release(assets=[])for _ in range(100)],page2=[release()]);add('best-compatible','available',rels=[release('new'),release('old')],metadataByTag={'new':metadata(package={**current,'version':'0.5.6-beta6'}),'old':metadata()})
requests=[]
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  name,kind,*rest=self.path.strip('/').split('/');c=cases[name];requests.append(self.path)
  if c.get('delay'):time.sleep(c['delay'])
  if c.get('redirect'):
   self.send_response(302);self.send_header('Location',c['redirect']);self.end_headers();return
  status=c.get('http',200);self.send_response(status);self.send_header('Content-Type','application/json');self.end_headers()
  if kind=='releases':body=json.dumps(c.get('page2',[]) if 'page2' in c and len([x for x in requests if x==self.path])>1 else c['releases'])
  else:body=c.get('raw',json.dumps(c.get('metadataByTag',{}).get(rest[0],c['metadata'])))
  if c.get('huge'):body='x'*(2*1024*1024+100)
  try:self.wfile.write(body.encode())
  except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError):pass
class Server(ThreadingHTTPServer):
 def handle_error(self,*args):pass # Deliberate timeout/cancellation disconnects; assertions remain active.
server=Server(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();results=[]
try:
 for name,c in cases.items():
  src=work/(name+'.input.json');dst=work/(name+'.result.json');src.write_text(json.dumps({'current':c.get('currentOverride',current),'cancel':c.get('cancel',False)}));endpoint=f'http://127.0.0.1:{server.server_port}/{name}'
  if c.get('offline'):endpoint='http://127.0.0.1:1/'+name
  p=subprocess.run([str(ROOT/'build/native-installer/OnlineUpdateTest.exe'),str(src),endpoint,str(dst)],capture_output=True,timeout=15);assert p.returncode==0,(name,p.stderr)
  result=json.loads(dst.read_text(encoding='utf-8-sig'));assert result['status']==c['expected'],(name,result,c['expected']);assert result['elapsedMs']<12000
  if result['status']=='available':assert result['url'].startswith(repo+'releases/tag/')
  results.append({'case':name,'status':'PASS','result':result});print(name+': PASS',flush=True)
 if args.legacy_binary:
  cases['legacy-client']=copy.deepcopy(cases['legacy-wire-display'])
  src=work/'legacy-client.input.json';dst=work/'legacy-client.result.json';src.write_text(json.dumps({'current':current}))
  run=subprocess.run([str(args.legacy_binary),str(src),f'http://127.0.0.1:{server.server_port}/legacy-client',str(dst)],capture_output=True,timeout=15)
  old_result=json.loads(dst.read_text(encoding='utf-8-sig'));assert run.returncode==0 and old_result['status']=='available' and old_result['version']=='0.5.5-beta5',old_result
  results.append({'case':'actual-previous-220-client','status':'PASS','result':old_result})
 result={'status':'PASS','checks':len(results),'results':results,'transport':'Actual WinHTTP to loopback mock server','publicNetworkUsed':False,'fixture':str(work)}
except Exception as e:result={'status':'FAIL','checks':len(results),'error':str(e),'fixture':str(work)};raise
finally:
 server.shutdown();server.server_close();(ROOT/'build/online-update-results.json').write_text(json.dumps(result,indent=2)+'\n')
