"""Run multi-voice generation around occupied channels and densely packed steps."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'toern_helpers.ino').read_text()
def function(signature):
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
code=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
using std::min;using std::max;
#define FLASHMEM
#define constrain(v,a,b) std::clamp((v),(a),(b))
constexpr unsigned maxX=16,MAX_STEPS=64;
constexpr uint8_t NOTE_MIDI_PITCH_NONE=255;
struct Note {uint8_t channel=0,velocity=127,probability=100,condition=1,midiPitch=255;};
#define MATRIX_WIDTH 16
#define maxY 16
Note note[65][17];
Note pageTmp[(MATRIX_WIDTH * 2) + 1][maxY + 1];
struct {int edit=2;} GLOB;
uint32_t seed=1;
int random(int high){seed=1664525u*seed+1013904223u;return (seed>>1)%high;}
int random(int low,int high){return low+random(high-low);}
int companionEffectivePages(){return 4;}int companionActivePages(){return 1;}
int updates=0,generated=0,contextsWithExisting=0;
void updateLastPage(){++updates;}
bool isChildVoiceDisabled(int ch){return ch==14;}
struct CompanionContext {int count=0;};struct CompanionHarmony{};struct CompanionGroove{};
CompanionContext analyzeCompanionContext(int first,int last,int focus,uint8_t excluded){
 assert(first==1&&last>=focus);int count=0;
 for(unsigned step=(focus-1)*16+1;step<=unsigned(focus)*16;++step)
  for(int row=1;row<=16;++row)if(note[step][row].channel&&note[step][row].channel!=excluded)++count;
 if(count)++contextsWithExisting;return {count};
}
CompanionHarmony companionMakeHarmony(const CompanionContext&,int){return {};}
CompanionGroove companionMakeGroove(const CompanionContext&,int,int){return {};}
bool generationUndoValid=false;unsigned generationUndoPage=0;unsigned generationUndoWidth=0;
'''
for signature in ['static inline uint8_t clampVel(', 'static inline void placeGenNote(',
                  'FLASHMEM static int companionFreeRow(unsigned int step, int preferred) {',
                  'FLASHMEM static int companionChannelStepLimit(', 'FLASHMEM static bool companionPlaceLimited(',
                  'FLASHMEM static void captureGenerationUndo(', 'FLASHMEM bool undoLastGeneration()']:
    code+=function(signature)+'\n'
code+=r'''
void generateCompanionChannelPage(int page,uint8_t ch,const CompanionContext &ctx,const CompanionHarmony&,const CompanionGroove&,int){
 assert(ch!=14);++generated;
 // Use the actual empty-cell placement path against collisions on every step.
 for(unsigned step=(page-1)*16+1;step<=unsigned(page)*16;++step)
  companionPlaceLimited(step,6,ch,80+ctx.count%30);
}
'''+function('FLASHMEM void drawRandomsMultiPage()')+r'''
int main(){
 for(unsigned trial=1;trial<=200;++trial){
  for(auto &step:note)for(auto &cell:step)cell=Note{};
  seed=trial;generated=contextsWithExisting=0;
  note[17][6]={1,77,35,22,60};note[20][8]={6,81,62,7,41};
  note[22][5]={11,65,24,2,72};note[29][13]={13,99,88,3,127};
  note[2][4]={2,49,11,8,36}; // A different page is never changed.
  Note before[65][17];memcpy(before,note,sizeof(note));drawRandomsMultiPage();
  assert(contextsWithExisting>0&&generated>0);
  const unsigned protectedChannels=(1u<<1)|(1u<<6)|(1u<<11)|(1u<<13);
  for(unsigned step=1;step<=64;++step)for(unsigned row=1;row<=16;++row){
   const auto &old=before[step][row];const auto &now=note[step][row];
   if(step<17||step>32||old.channel)assert(memcmp(&old,&now,sizeof(Note))==0);
   if(!old.channel&&now.channel)assert(!(protectedChannels&(1u<<now.channel)));
  }
  assert(undoLastGeneration());
  assert(memcmp(before,note,sizeof(note))==0);
  assert(!undoLastGeneration());
 }
 // Fully occupied rows must not be overwritten to make room for a new voice.
 for(unsigned step=17;step<=32;++step)for(unsigned row=1;row<=16;++row)
   note[step][row]={1,60,40,3,64};
 Note before[65][17];memcpy(before,note,sizeof(note));drawRandomsMultiPage();
 assert(memcmp(before,note,sizeof(note))==0);
 assert(undoLastGeneration());
 assert(memcmp(before,note,sizeof(note))==0);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory);(path/'generate.cpp').write_text(code)
    flags=['c++','-std=c++17','-fsanitize=address,undefined']
    sdk=Path('/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1')
    if sdk.exists():flags+=['-isystem',str(sdk)]
    subprocess.run(flags+[str(path/'generate.cpp'),'-o',str(path/'generate')],check=True)
    subprocess.run([str(path/'generate')],check=True)
print('PASS: generation preserves complete existing channels/metadata, uses their context, respects other pages and never overwrites full steps')
