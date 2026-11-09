"""Compare every firmware/simulator activity-icon pixel, including late preparation."""
from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
source='''#include <cstdio>
#include "src/toern_autogen.h"
int main(){for(bool ready:{false,true})for(unsigned progress=0;progress<=12;++progress)
 for(unsigned frame=0;frame<8;++frame)for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x)
  putchar('0'+ToernAutogen::statusInk(x,y,progress,ready,frame*240));}
'''.replace('#include <cstdio>', '#include <cstdio>\n#include <initializer_list>')
with tempfile.TemporaryDirectory(prefix='agen-status-parity-') as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(source)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,'-I',str(root/'platformio'),str(cpp),'-o',str(exe)],check=True)
 firmware=subprocess.check_output([str(exe)])
 simulator=subprocess.check_output(['node','--input-type=module','-e', '''
import {statusInk} from './tools/sim/src/firmware/autogen.js';
let pixels='';for(const ready of [false,true])for(let progress=0;progress<=12;progress++)
 for(let frame=0;frame<8;frame++)for(let y=0;y<5;y++)for(let x=0;x<5;x++)pixels+=statusInk(x,y,progress,ready,frame*240);
process.stdout.write(pixels);
'''],cwd=root)
 assert firmware==simulator,'Device and simulator AGEN icons differ'
print(f'PASS: {len(firmware)} icon pixels match across all progress, ready/late and animation states')
