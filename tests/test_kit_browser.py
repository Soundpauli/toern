"""KIT browsing: immediate selection, bounded SD work, cached empty/sparse slots."""
from pathlib import Path
import subprocess,sys,tempfile
root=Path(__file__).resolve().parents[1]
code=r'''
#include "src/toern_kit_browser.h"
#include "src/toern_loop_timing.h"
#include <cassert>
#include <cstdio>
int main(){
 ToernLoopTiming timing;
 timing.begin(100);timing.mark(ToernLoopTiming::Storage,200);timing.end(60200);
 assert(timing.maximum[ToernLoopTiming::Serial]==100&&timing.loopMaximum==60100);
 assert(timing.stalls==1&&timing.lastStallStage==ToernLoopTiming::Storage&&timing.lastStallUs==60000);
 timing.begin(0xfffffff0);timing.end(0x10);assert(timing.stalls==1);
 ToernKitBrowser b;unsigned calls=0;
 auto empty=[&](unsigned slot,unsigned voice){assert(slot==999);assert(voice==calls+1);++calls;return false;};
 assert(b.select(999,100));assert(b.pending()&&!b.available());
 b.tick(134,empty);assert(calls==0);
 for(unsigned i=0;i<8;++i){b.tick(135+i*100,empty);assert(calls==i+1);}
 assert(!b.pending()&&!b.available());
 for(unsigned i=0;i<20;++i)b.tick(10000+i*100,empty);
 assert(calls==8); // No time-gap invalidation after slow frames.
 assert(b.select(11,20000));assert(b.pending());
 assert(b.select(999,20001));assert(!b.pending());b.tick(30000,empty);assert(calls==8);
 b.select(11,31000);unsigned voices=0;
 for(unsigned i=0;i<8&&b.pending();++i)b.tick(31035+i,[&](unsigned slot,unsigned voice){assert(slot==11);++voices;return voice==3;});
 assert(voices==3&&b.available()); // A kit need not contain voice 1.
 b.select(12,32000);b.select(13,32020);
 b.tick(32040,[&](unsigned,unsigned){assert(false);return false;}); // Skip numbers scrolled past.
 b.reset();b.select(999,33000);assert(b.pending()); // Re-entry/save invalidates stale results.
 puts("PASS: immediate KIT selection, settled/bounded probes, empty and sparse slots, cache across slow frames, re-entry invalidation");
}
'''
with tempfile.TemporaryDirectory(prefix='toern-kit-browser-') as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test';cpp.write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-fsanitize=address,undefined',*flags,'-I',str(root),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
s=(root/'toern.ino').read_text();render=s[s.index('FLASHMEM void showSamplePack()'):s.index('FLASHMEM void loadSamplePack(')]
assert 'lastCallMs' not in render
assert 'browser.tick' in render and 'browser.select' in render
