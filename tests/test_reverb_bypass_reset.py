"""Exercise the firmware RVRB action on every playable channel."""
from pathlib import Path
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'toern_filter.ino').read_text()
a=source.index('FLASHMEM void bypassAllReverbAndBitcrushers()')
b=source.index('\nvoid setFilterDefaults',a)
code=r"""
#include <cassert>
#include <cstring>
#define FLASHMEM
const int ALL_CHANNELS[]={1,2,3,4,5,6,7,8,11,13,14};
const int NUM_ALL_CHANNELS=11,REVERB=2,BITCRUSHER=3;
struct Settings {float filter_settings[15][12];int channelVol[15];} SMP;
int applied[15][12]={};
void setFilters(int type,int ch,bool initial){
 assert(ch>=1&&ch<15&&initial);
 assert(type==REVERB||type==BITCRUSHER);
 assert(SMP.filter_settings[ch][type]==0);
 ++applied[ch][type];
}
FUNCTION
int main(){
 for(int ch=0;ch<15;++ch){SMP.channelVol[ch]=ch;for(int p=0;p<12;++p)SMP.filter_settings[ch][p]=ch*12+p+1;}
 Settings before=SMP;
 bypassAllReverbAndBitcrushers();
 for(int ch:ALL_CHANNELS){
  assert(applied[ch][REVERB]==1&&applied[ch][BITCRUSHER]==1);
  before.filter_settings[ch][REVERB]=before.filter_settings[ch][BITCRUSHER]=0;
 }
 assert(memcmp(&SMP,&before,sizeof(SMP))==0);
 bypassAllReverbAndBitcrushers();assert(memcmp(&SMP,&before,sizeof(SMP))==0);
}
""".replace('FUNCTION',source[a:b])
menu=(root/'toern_menu.ino').read_text()
assert '#define LOOK_PAGES_COUNT 5' in menu and '#define ETC_PAGES_COUNT 5' in menu
look=menu[menu.index('MenuPage lookPages'):menu.index('static inline uint16_t pongSpeedToInterval')]
etc=menu[menu.index('MenuPage etcPages'):menu.index('MenuPage modePages')]
assert look.index('"MUTE"')<look.index('"SWAP"') and '"SWAP"' not in etc
assert 'resetMenuOption == 7' in menu and 'modeText = "RVRB"' in menu
with tempfile.TemporaryDirectory(prefix='toern-rvrb-') as tmp:
 tmp=Path(tmp);(tmp/'test.cpp').write_text(code)
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17',*flags,str(tmp/'test.cpp'),'-o',str(tmp/'test')],check=True)
 subprocess.run([str(tmp/'test')],check=True)
print('PASS: RVRB zeros/applies both effects on all 11 voices, preserves other parameters/volumes, repeats safely; SWAP follows MUTE in SETT.')
