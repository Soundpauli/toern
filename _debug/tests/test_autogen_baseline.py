"""Keep the musical policy requested from 9671a48 while allowing live source edits."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2]
p='platformio/toern_helpers.ino'
current=(root/p).read_text();baseline=subprocess.check_output(['git','show',f'9671a48:{p}'],cwd=root,text=True)
def function(text,name):
 start=text.index(name+'(');start=text.rfind('\n',0,start)+1
 # Skip a forward declaration where applicable.
 while text.find(';',start)<text.find('{',start):
  start=text.index(name+'(',text.index(';',start)+1);start=text.rfind('\n',0,start)+1
 end=text.index('{',start)+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
for name in ['evolveCompanionChannelFromBase','generateCompanionChannelPage','applyAutogenProbabilities']:
 assert function(current,name)==function(baseline,name),name+' changed from the requested musical policy'
for path in ['platformio/src/toern_autogen_music.h','platformio/src/toern_autogen.h']:
 assert (root/path).read_bytes()==subprocess.check_output(['git','show',f'9671a48:{path}'],cwd=root),path
print('PASS: evolution, companion generation,  probability, repeats and section timing match 9671a48 exactly; register, chord-density and mono-phrase refinements are tested separately.')
