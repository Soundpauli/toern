#include "src/toern_pattern_io.h"
using SubpatternReader = toern_pattern_io::Reader<File,Note,Device,MAX_STEPS,maxY>;
using SubpatternWriter = toern_pattern_io::Writer<File,Note,Device,MAX_STEPS,maxY>;
using SubpatternIoResult = toern_pattern_io::Result;
struct SubpatternMetadata {
  uint8_t length[16]{},longest=1;
  uint16_t pages[16]{},rows[16]{},mask=0;
  bool has[maxPages+1]{};
  unsigned last=1;
};
static SubpatternMetadata subpatternMetadata[5],subpatternBuilding;
static Note (*subpatternCache[5])[maxY+1] = {};
static uint8_t subpatternDirtyMask=0,subpatternEmptyMask=0;
static unsigned long subpatternErrorUntil=0,subpatternRetryAt=0,subpatternSuccessUntil=0;
static bool subpatternSwitchArmed=false;
static bool subpatternGridReady=false,subpatternViewOn=false,subpatternViewDirty=false;
static uint8_t subpatternGridSlot=255,subpatternViewSlot=0;
static unsigned subpatternViewPage=1,subpatternSelectedPage=0;
static SubpatternReader subpatternReadJob;
static SubpatternWriter subpatternWriteJob;
static uint8_t subpatternWritingSlot=255,subpatternLoadingSlot=255,subpatternCandidate=0;
static bool subpatternSawInvalid=false,subpatternRecovered=false;
static unsigned subpatternClearCell=0,subpatternMetaCell=0,subpatternLoadWidth=16;
enum SubpatternLoadPhase { SubLoadIdle,SubLoadClear,SubLoadRead,SubLoadMeta };
static SubpatternLoadPhase subpatternLoadPhase=SubLoadIdle;

// Root index is cooperative too. The renderer reads the current cache only.
static DMAMEM uint8_t subpatternFileMasks[1000];
static File subpatternIndexRoot;
static uint32_t subpatternIndexBrowser=~0u,subpatternIndexStorage=~0u;

static bool subpatternIndexBusy=false,subpatternIndexValid=false;

FLASHMEM static unsigned loadedPatternFile() {
  return subpatternFile>=1&&subpatternFile<=999?subpatternFile:SMP.file;
}
FLASHMEM static void subpatternFailure(){subpatternSuccessUntil=0;subpatternErrorUntil=millis()+1200;}
FLASHMEM static void subpatternCancelPreview() {
  subpatternReadJob.cancel();subpatternLoadPhase=SubLoadIdle;subpatternLoadingSlot=255;
  subpatternViewOn=false;subpatternViewDirty=false;subpatternSwitchArmed=false;
  subpatternGridReady=false;subpatternGridSlot=255;subpatternSelectedPage=0;
}
FLASHMEM static void resetFileSubpattern() {
  subpatternWriteJob.abandon(SD);subpatternWritingSlot=255;
  subpatternReady=false;subpatternApplied=false;subpatternSelector=false;
  subpatternFile=0;pendingNewPatternFile=0;deletedSubpatternSlots=0;subpatternSlot=0;
  subpatternSaveNew=false;subpatternOldSavePending=false;
  subpatternDirtyMask=0;subpatternEmptyMask=0;subpatternAvailable=0;
  memset(subpatternCache,0,sizeof(subpatternCache));
  subpatternCancelPreview();
}
FLASHMEM static void beginNewPattern() {
  const unsigned target=pendingNewPatternFile?pendingNewPatternFile:loadedPatternFile();
  resetFileSubpattern();SMP.file=target;subpatternFile=target;
}
FLASHMEM static uint8_t savedSubpatternMask(unsigned file) {
  return file>=1&&file<=999?subpatternFileMasks[file]:0;
}
FLASHMEM static void serviceSubpatternIndex() {
  if(currentMode!=&loadSaveTrack){if(subpatternIndexRoot)subpatternIndexRoot.close();subpatternIndexBusy=false;return;}
  if(subpatternIndexBrowser!=patternBrowserRevision||subpatternIndexStorage!=patternStorageRevision) {
    if(subpatternIndexRoot)subpatternIndexRoot.close();
    memset(subpatternFileMasks,0,sizeof(subpatternFileMasks));
    subpatternIndexBrowser=patternBrowserRevision;subpatternIndexStorage=patternStorageRevision;
    subpatternIndexBusy=true;subpatternIndexValid=false;++subpatternIndexPaintRevision;
    subpatternIndexRoot=SD.open("/");
    if(!subpatternIndexRoot){subpatternIndexBusy=false;subpatternFailure();}
    return;
  }
  if(!subpatternIndexBusy)return;
  File entry=subpatternIndexRoot.openNextFile();
  if(!entry){subpatternIndexRoot.close();subpatternIndexBusy=false;subpatternIndexValid=true;++subpatternIndexPaintRevision;return;}
  unsigned number;uint8_t slot;char canonical[32];
  snprintf(canonical,sizeof(canonical),"%s",entry.name());
  const size_t len=strlen(canonical);
  if(len>4&&(!strcmp(canonical+len-4,".tmp")||!strcmp(canonical+len-4,".bak")
             ||!strcmp(canonical+len-4,".TMP")||!strcmp(canonical+len-4,".BAK")))canonical[len-4]=0;
  if(!entry.isDirectory()&&toern_pattern_slots::parse(canonical,number,slot)) {
    const uint8_t before=subpatternFileMasks[number];subpatternFileMasks[number]|=1u<<slot;
    const unsigned selected=currentMode->pos[3]>1?currentMode->pos[3]-1:0;
    if(before!=subpatternFileMasks[number]&&(number==selected||number==loadedPatternFile()))++subpatternIndexPaintRevision;
  }
  entry.close();
}
// Occupancy and file presence are separate: empty trailing files still exist
// for loading/overwrite/deletion, but do not extend the displayed letter count.
static DMAMEM uint8_t subpatternContentMasks[1000],subpatternContentKnown[1000];
static uint32_t subpatternContentRevision=~0u;
static SubpatternReader subpatternContentReader;
static unsigned subpatternContentFile=0;
static uint8_t subpatternContentSlot=0,subpatternContentCandidate=0;
static bool subpatternContentReading=false;
FLASHMEM static bool subpatternRamHasNotes(const Note (*grid)[maxY+1]) {
  for(unsigned x=1;x<maxlen;++x)for(unsigned y=1;y<=maxY;++y)
    if(grid[x][y].channel>=1&&grid[x][y].channel<=16)return true;
  return false;
}
FLASHMEM static uint8_t subpatternContentMask(unsigned file) {
  if(file<1||file>999)return 0;
  uint8_t mask=subpatternContentRevision==patternStorageRevision?subpatternContentMasks[file]:0;
  if(file==loadedPatternFile())for(uint8_t slot=0;slot<5;++slot)if(subpatternCache[slot]) {
    if(subpatternRamHasNotes(subpatternCache[slot]))mask|=1u<<slot;
    else mask&=~(1u<<slot);
  }
  return mask;
}
FLASHMEM static void cancelSubpatternContentRead() {
  subpatternContentReader.cancel();subpatternContentReading=false;
}
FLASHMEM static bool serviceSubpatternContentCounts() {
  if(currentMode!=&loadSaveTrack){cancelSubpatternContentRead();return false;}
  if(subpatternContentRevision!=patternStorageRevision) {
    cancelSubpatternContentRead();memset(subpatternContentMasks,0,sizeof(subpatternContentMasks));
    memset(subpatternContentKnown,0,sizeof(subpatternContentKnown));subpatternContentRevision=patternStorageRevision;
    ++subpatternIndexPaintRevision;return true;
  }
  const unsigned selected=currentMode->pos[3]>1?currentMode->pos[3]-1:0;
  const unsigned wanted=selected&&savedSubpatternMask(selected)?selected:loadedPatternFile();
  if(wanted<1||wanted>999){cancelSubpatternContentRead();return false;}
  if(subpatternContentReading&&subpatternContentFile!=wanted){cancelSubpatternContentRead();return true;}
  if(subpatternContentReading) {
    const auto result=subpatternContentReader.tick(SD);if(result==SubpatternIoResult::Running)return true;
    if(result!=SubpatternIoResult::Valid&&++subpatternContentCandidate<3) {
      char base[24],path[32];toern_pattern_slots::path(base,sizeof(base),wanted,subpatternContentSlot);
      snprintf(path,sizeof(path),"%s.%s",base,subpatternContentCandidate==1?"tmp":"bak");
      subpatternContentReader.begin(path,subpatternContentCandidate==1);return true;
    }
    const uint8_t bit=1u<<subpatternContentSlot;
    if(result==SubpatternIoResult::Valid&&subpatternContentReader.hasNotes())subpatternContentMasks[wanted]|=bit;
    else subpatternContentMasks[wanted]&=~bit;
    subpatternContentKnown[wanted]|=bit;subpatternContentReading=false;++subpatternIndexPaintRevision;return true;
  }
  for(uint8_t slot=0;slot<5;++slot) {
    const uint8_t bit=1u<<slot;if(subpatternContentKnown[wanted]&bit)continue;
    if(wanted==loadedPatternFile()&&subpatternCache[slot]) {
      if(subpatternRamHasNotes(subpatternCache[slot]))subpatternContentMasks[wanted]|=bit;
      else subpatternContentMasks[wanted]&=~bit;
      subpatternContentKnown[wanted]|=bit;++subpatternIndexPaintRevision;return true;
    }
    if(!(savedSubpatternMask(wanted)&bit)){subpatternContentKnown[wanted]|=bit;continue;}
    char path[24];toern_pattern_slots::path(path,sizeof(path),wanted,slot);
    subpatternContentFile=wanted;subpatternContentSlot=slot;subpatternContentCandidate=0;
    subpatternContentReader.begin(path,false);subpatternContentReading=true;return true;
  }
  return false;
}

FLASHMEM static void drawPatternSlotDots(uint8_t mask,CRGB color) {
  const unsigned count=toern_pattern_slots::displayCount(mask);
  const int start=maxX>=32?10:5;
  for(unsigned i=0;i<count;++i)light(start+2*i,7,color);
}
FLASHMEM static void patternDestination(char *out,size_t size,unsigned file) {
  const uint8_t slot=subpatternFile==file?(subpatternSavingPrevious?subpatternPreviousSlot:subpatternSlot):0;
  if(!toern_pattern_slots::path(out,size,file,slot))out[0]=0;
}
FLASHMEM static Note (*subpatternFreeBank())[maxY+1] {
  for(auto &bank:noteBanks) {
    Note (*candidate)[maxY+1]=bank;bool used=candidate==note;
    for(auto cached:subpatternCache)used|=candidate==cached;
    if(!used)return candidate;
  }
  return nullptr;
}
FLASHMEM static void subpatternAddMetadata(SubpatternMetadata &meta,unsigned cell,const Note &value) {
  if(!value.channel||maxX==0)return;
  const unsigned x=cell/maxY+1,y=cell%maxY+1,page=(x-1)/maxX+1;
  if(page>maxPages||page>(unsigned)effectivePageCount())return;
  meta.has[page]=true;meta.mask|=1u<<(page-1);meta.last=std::max(meta.last,page);
  if(value.channel<16){const unsigned ch=value.channel;meta.length[ch]=page;
    meta.pages[ch]|=1u<<(page-1);meta.rows[ch]|=1u<<(y-1);meta.longest=std::max(meta.longest,(uint8_t)page);}
}
FLASHMEM static void subpatternStartLoad(uint8_t slot) {
  subpatternReadJob.cancel();subpatternLoadingSlot=slot;subpatternGridReady=false;subpatternGridSlot=255;
  subpatternLoadWidth=maxX;subpatternCandidate=0;subpatternSawInvalid=false;subpatternRecovered=false;
  subpatternBuilding=SubpatternMetadata{};subpatternMetaCell=0;
  if(subpatternCache[slot]) {subpatternGrid=subpatternCache[slot];subpatternLoadPhase=SubLoadMeta;return;}
  subpatternGrid=subpatternFreeBank();
  if(!subpatternGrid){subpatternLoadPhase=SubLoadIdle;subpatternFailure();return;}
  subpatternClearCell=0;subpatternLoadPhase=SubLoadClear;
}
FLASHMEM static void serviceSubpatternLoad() {
  if(subpatternLoadPhase==SubLoadIdle)return;
  if(subpatternLoadWidth!=maxX){const uint8_t slot=subpatternLoadingSlot;subpatternStartLoad(slot);return;}
  if(subpatternLoadPhase==SubLoadClear) {
    // Fixed-size RAM2 initialization only; interrupts remain enabled.
    const unsigned end=(maxlen+1)*(maxY+1);
    for(;subpatternClearCell<end;++subpatternClearCell)
      subpatternGrid[subpatternClearCell/(maxY+1)][subpatternClearCell%(maxY+1)]={0,defaultVelocity,100,1,NOTE_MIDI_PITCH_NONE};
    if(subpatternClearCell==(maxlen+1)*(maxY+1)) {
      char path[24];toern_pattern_slots::path(path,sizeof(path),subpatternFile,subpatternLoadingSlot);
      subpatternReadJob.begin(path,false,subpatternGrid);subpatternLoadPhase=SubLoadRead;
    }
    return;
  }
  if(subpatternLoadPhase==SubLoadRead) {
    const auto result=subpatternReadJob.tick(SD);if(result==SubpatternIoResult::Running)return;
    if(result==SubpatternIoResult::Valid) {
      subpatternRecovered=subpatternCandidate>0;subpatternEmptyMask&=~(1u<<subpatternLoadingSlot);
      subpatternLoadPhase=SubLoadMeta;return;
    }
    subpatternSawInvalid|=result!=SubpatternIoResult::Missing;
    if(++subpatternCandidate<3) {
      char base[24],path[32];toern_pattern_slots::path(base,sizeof(base),subpatternFile,subpatternLoadingSlot);
      snprintf(path,sizeof(path),"%s.%s",base,subpatternCandidate==1?"tmp":"bak");
      subpatternReadJob.begin(path,subpatternCandidate==1,subpatternGrid);return;
    }
    if(subpatternSawInvalid){subpatternCancelPreview();subpatternSelector=false;subpatternFailure();return;}
    subpatternEmptyMask|=1u<<subpatternLoadingSlot;subpatternLoadPhase=SubLoadMeta;return;
  }
  // Metadata is computed in the main loop over fixed-size RAM2, never in PIT.
  const unsigned end=MAX_STEPS*maxY;
  for(;subpatternMetaCell<end;++subpatternMetaCell)
    subpatternAddMetadata(subpatternBuilding,subpatternMetaCell,subpatternGrid[1+subpatternMetaCell/maxY][1+subpatternMetaCell%maxY]);
  if(subpatternMetaCell==MAX_STEPS*maxY) {
    if(!voiceMode&&loopLength>0)subpatternBuilding.last=std::min(loopLength,effectivePageCount());
    const uint8_t slot=subpatternLoadingSlot;
    subpatternMetadata[slot]=subpatternBuilding;subpatternCache[slot]=subpatternGrid;
    if(!(subpatternEmptyMask&(1u<<slot)))subpatternAvailable|=1u<<slot;
    if(subpatternRecovered)subpatternDirtyMask|=1u<<slot;
    subpatternGridSlot=slot;subpatternGridReady=true;subpatternLoadPhase=SubLoadIdle;
    subpatternViewDirty=true;
  }
}
FLASHMEM static void subpatternCaptureSettings(Device &settings,unsigned file) {
  memcpy(&settings,&SMP,sizeof(settings));settings.file=file;
  for(unsigned ch=0;ch<maxY;++ch){settings.globalMutes[ch]=globalMutes[ch];
    for(unsigned page=0;page<maxPages;++page)settings.pageMutes[page][ch]=pageMutes[page][ch];}
}
FLASHMEM static void subpatternShowVisible(uint8_t slot) {
  if(slot>=5||!subpatternCache[slot])return;
  unsigned page=subpatternSelectedPage?subpatternSelectedPage:std::max(GLOB.edit,1u);
  const uint16_t mask=subpatternMetadata[slot].mask;
  if(page>maxPages||!(mask&(1u<<(page-1)))) {
    page=1;for(unsigned p=1;p<=maxPages;++p)if(mask&(1u<<(p-1))){page=p;break;}
  }
  subpatternSelectedPage=page;subpatternViewPage=page;subpatternViewSlot=slot;subpatternViewOn=true;subpatternViewDirty=false;
}
FLASHMEM static bool subpatternTurn(int delta) {
  if(!subpatternsEnabled||fastRecordActive||isRecording||childLockEnabled||imageMode||subpatternReady
      ||subpatternApplied||patternFamilyBusy()||subpatternFile>999||loadedPatternFile()<1)return true;
  if(!subpatternSelector){
    subpatternErrorUntil=0;subpatternSuccessUntil=0;
    if(!subpatternFile){subpatternFile=loadedPatternFile();subpatternSlot=0;subpatternNotePagesLoaded();}
    subpatternSelected=subpatternSlot;subpatternSelectedPage=0;subpatternSelector=true;
  }
  const uint8_t target=toern_pattern_slots::select(subpatternSelected,delta<0?-1:delta>0?1:0);
  if(target!=subpatternSelected||!subpatternGridReady){subpatternSelected=target;subpatternSelectedPage=0;
    subpatternViewOn=false;subpatternStartLoad(target);}
  return true;
}
FLASHMEM static bool subpatternPageTurn(int delta) {
  if(!subpatternSelector||!subpatternGridReady||delta==0)return true;
  const uint16_t mask=subpatternMetadata[subpatternSelected].mask;
  int current=subpatternSelectedPage?subpatternSelectedPage:1,step=delta<0?-1:1;
  for(int page=current+step;page>=1&&page<=maxPages;page+=step)if(mask&(1u<<(page-1))) {
    subpatternSelectedPage=page;subpatternShowVisible(subpatternSelected);break;
  }
  return true;
}
FLASHMEM static bool subpatternOverlayVisible() {
  return (currentMode==&draw||currentMode==&loadSaveTrack) &&
    ((currentMode==&draw&&((fnHeld&&subpatternSelector)||subpatternSwitchArmed||subpatternReady))||patternFamilyBusy()
      ||(int32_t)(subpatternErrorUntil-millis())>0||(int32_t)(subpatternSuccessUntil-millis())>0);
}
FLASHMEM static void drawSubpatternOverlay() {
  if(patternFamilyBusy()){drawCenteredOverlayLabel("WAIT",UI_BLUE);return;}
  if((int32_t)(subpatternSuccessUntil-millis())>0){drawCenteredOverlayLabel("OK",UI_GREEN);return;}
  if((int32_t)(subpatternErrorUntil-millis())>0){drawCenteredOverlayLabel("ERR",UI_RED);return;}
  char text[6];const unsigned page=std::min(std::max(subpatternSelectedPage,1u),(unsigned)maxPages);
  snprintf(text,sizeof(text),"%c%u",'A'+subpatternSelected,page);
  drawCenteredOverlayLabel(text,subpatternCache[subpatternSelected]&&!(subpatternEmptyMask&(1u<<subpatternSelected))?UI_GREEN:UI_BLUE);
}
FLASHMEM static bool subpatternFnRelease() {
  if(!subpatternSelector)return false;
  subpatternSelector=false;
  if(currentMode!=&draw){subpatternCancelPreview();return false;}
  if(subpatternSelected==subpatternSlot){
    const unsigned page=subpatternSelectedPage;
    if(page>=1&&page<=maxPages&&page!=GLOB.edit&&subpatternMetadata[subpatternSlot].has[page]){
      const unsigned col=maxX&&GLOB.x?((GLOB.x-1)%maxX+1):1;
      GLOB.edit=page;editpage=page;GLOB.x=(page-1)*maxX+col;
      if(!isNowPlaying){GLOB.page=page;beat=beatForUI=(page-1)*maxX+1;}
      subpatternCancelPreview();return true;
    }
    subpatternCancelPreview();return false;
  }
  subpatternPendingSlot=subpatternSelected;subpatternSwitchArmed=true;return false;
}

// Constant-size publication only: no SD, full-grid copy, scan, or encoder I/O.
static bool subpatternCommitAtStepStart() {
  if(!subpatternReady||fastRecordActive||isRecording||(isNowPlaying&&!subpatternBoundary))return false;
  subpatternPreviousSlot=subpatternSlot;subpatternOutgoingGrid=note;
  subpatternDirtyMask|=1u<<subpatternPreviousSlot;
  note=subpatternCache[subpatternPendingSlot];subpatternSlot=subpatternPendingSlot;
  const auto &meta=subpatternMetadata[subpatternSlot];
  memcpy(voicePageLen,meta.length,sizeof(voicePageLen));memcpy(voicePageMask,meta.pages,sizeof(voicePageMask));
  memcpy(voiceRowMask,meta.rows,sizeof(voiceRowMask));memcpy(hasNotes,meta.has,sizeof(hasNotes));
  voiceModePages=meta.longest;lastPage=meta.last;
  for(int ch=0;ch<16;++ch)voiceLoopCount[ch]=1;
  unsigned page=subpatternSelectedPage?subpatternSelectedPage:std::max(GLOB.edit,1u);
  if(page>maxPages||!hasNotes[page]){page=1;for(unsigned p=1;p<=maxPages;++p)if(hasNotes[p]){page=p;break;}}
  const unsigned col=maxX&&GLOB.x?((GLOB.x-1)%maxX+1):1;
  GLOB.page=GLOB.edit=page;editpage=page;GLOB.x=(page-1)*maxX+col;beat=beatForUI=(page-1)*maxX+1;
  loopCount=1;pendingPage=0;SMP.file=subpatternFile;
  if(subpatternEmptyMask&(1u<<subpatternSlot))subpatternDirtyMask|=1u<<subpatternSlot;
  subpatternReady=false;subpatternBoundary=false;subpatternSwitchArmed=false;subpatternViewOn=false;
  subpatternGridReady=false;subpatternSelectedPage=0;__DMB();subpatternApplied=true;return true;
}
void subpatternNotePagesLoaded() {
  subpatternCancelPreview();subpatternAvailable=0;subpatternEmptyMask=0;subpatternDirtyMask=0;
  memset(subpatternCache,0,sizeof(subpatternCache));subpatternCache[subpatternSlot]=note;
  auto &meta=subpatternMetadata[subpatternSlot];meta=SubpatternMetadata{};
  memcpy(meta.length,voicePageLen,sizeof(voicePageLen));memcpy(meta.pages,voicePageMask,sizeof(voicePageMask));
  memcpy(meta.rows,voiceRowMask,sizeof(voiceRowMask));memcpy(meta.has,hasNotes,sizeof(hasNotes));
  meta.longest=voiceModePages;meta.last=lastPage;
  for(unsigned p=1;p<=maxPages;++p)if(hasNotes[p])meta.mask|=1u<<(p-1);
  subpatternAvailable|=1u<<subpatternSlot;
}
bool subpatternBrowsePageMask(uint16_t *mask) {
  const uint8_t slot=subpatternSelector?subpatternSelected:subpatternViewSlot;
  if(!mask||!subpatternViewOn||slot>=5||!subpatternCache[slot])return false;
  *mask=subpatternMetadata[slot].mask;return true;
}
bool subpatternPreviewVisible(){return subpatternViewOn&&subpatternCache[subpatternViewSlot];}
Note *subpatternViewedNote(unsigned ix,unsigned iy) {
  if(!subpatternPreviewVisible()||ix<1||ix>maxX||iy<1||iy>maxY)return nullptr;
  const unsigned x=(subpatternViewPage-1)*maxX+ix;if(x>=maxlen)return nullptr;
  return &subpatternCache[subpatternViewSlot][x][iy];
}
bool subpatternViewedShadowed(unsigned ix,unsigned iy) {
  if(!subpatternPreviewVisible())return false;
  const unsigned x=(subpatternViewPage-1)*maxX+ix;
  return toern_grid_paint::canReplaceShadowed(subpatternCache[subpatternViewSlot],x,iy,0,MAX_STEPS,maxY);
}
bool subpatternShowingOtherSlot(){return isNowPlaying&&subpatternPreviewVisible()&&subpatternViewSlot!=subpatternSlot;}

FLASHMEM static void stageSelectedSubpattern() {
  if(subpatternReady)return;
  if(!subpatternsEnabled||fastRecordActive||isRecording||subpatternWriteJob.busy())return;
  if(!subpatternSelector&&!subpatternSwitchArmed)return;
  const uint8_t target=subpatternSwitchArmed?subpatternPendingSlot:subpatternSelected;
  if(target>=5)return;
  if(subpatternLoadingSlot!=target&&!subpatternGridReady)subpatternStartLoad(target);
  if(subpatternLoadPhase!=SubLoadIdle){serviceSubpatternLoad();return;}
  if(!subpatternGridReady||subpatternGridSlot!=target)return;
  if(subpatternViewDirty||!subpatternViewOn)subpatternShowVisible(target);
  if(!subpatternSwitchArmed)return;
  if(target==subpatternSlot){subpatternCancelPreview();return;}
  toern_audio::StateGuard guard;
  if(!subpatternSwitchArmed||!subpatternGridReady||subpatternGridSlot!=target||subpatternSlot==target)return;
  subpatternPendingSlot=target;__DMB();subpatternReady=true;
  if(!isNowPlaying)subpatternCommitAtStepStart();
}
FLASHMEM static void serviceSubpatterns() {
  if(currentMode!=&loadSaveTrack&&subpatternContentReading){cancelSubpatternContentRead();return;}
  if(patternFamilyBusy()){servicePatternFamilyJob();return;}
  if(subpatternSelector&&currentMode!=&draw){subpatternSelector=false;subpatternCancelPreview();}
  if((!subpatternsEnabled||childLockEnabled||imageMode)&&(subpatternSelector||subpatternSwitchArmed||subpatternReady)) {
    subpatternReady=false;subpatternSelector=false;subpatternCancelPreview();
  }
  if(subpatternReady&&!isNowPlaying){toern_audio::StateGuard guard;subpatternCommitAtStepStart();}
  if(subpatternApplied){
    subpatternApplied=false;
    if(currentMode==&draw||currentMode==&singleMode){currentMode->pos[3]=GLOB.x;Encoder[3].writeCounter((int32_t)GLOB.x);
      if(ctrlMode==0){currentMode->pos[1]=GLOB.edit;Encoder[1].writeMax((int32_t)encoderPageMax());Encoder[1].writeCounter((int32_t)GLOB.edit);}
      applyDrawSingleYEncoderLimits();}
  }
  if(subpatternWriteJob.busy()) {
    if(isNowPlaying||fastRecordActive||isRecording||subpatternSelector||subpatternSwitchArmed){
      subpatternWriteJob.abandon(SD);subpatternWritingSlot=255;
      subpatternOldSavePending=false;subpatternSaveNew=false;return;
    }
    const auto result=subpatternWriteJob.tick(SD);
    if(result==SubpatternIoResult::Valid){subpatternDirtyMask&=~(1u<<subpatternWritingSlot);
      subpatternAvailable|=1u<<subpatternWritingSlot;
      subpatternEmptyMask&=~(1u<<subpatternWritingSlot);deletedSubpatternSlots&=~(1u<<subpatternWritingSlot);
      ++patternStorageRevision;subpatternWritingSlot=255;subpatternOldSavePending=false;subpatternSaveNew=false;}
    else if(result==SubpatternIoResult::Failed){subpatternRetryAt=millis()+1500;subpatternFailure();
      subpatternWritingSlot=255;subpatternOldSavePending=false;subpatternSaveNew=false;}
    return;
  }
  if(subpatternSelector||subpatternSwitchArmed||subpatternReady){stageSelectedSubpattern();return;}
  if(!isNowPlaying&&!fastRecordActive&&!isRecording&&subpatternWriteJob.incompleteTemporary()
      &&(int32_t)(millis()-subpatternRetryAt)>=0){
    if(!subpatternWriteJob.abandon(SD)){subpatternRetryAt=millis()+1500;subpatternFailure();}
    return;
  }
  if(!isNowPlaying&&!fastRecordActive&&!isRecording&&subpatternDirtyMask&&(int32_t)(millis()-subpatternRetryAt)>=0) {
    for(uint8_t slot=0;slot<5;++slot)if((subpatternDirtyMask&(1u<<slot))&&subpatternCache[slot]) {
      // Explicit deletion must not be undone by a deferred empty-grid save.
      if(deletedSubpatternSlots&(1u<<slot)) {
        bool blank=true;for(unsigned x=1;x<maxlen&&blank;++x)for(unsigned y=1;y<=maxY;++y)if(subpatternCache[slot][x][y].channel){blank=false;break;}
        if(blank){subpatternDirtyMask&=~(1u<<slot);continue;}
      }
      char path[24];toern_pattern_slots::path(path,sizeof(path),subpatternFile,slot);
      Device settings;subpatternCaptureSettings(settings,subpatternFile);
      subpatternWriteJob.begin(path,subpatternCache[slot],settings,slot,0,FILE_WRITE_BEGIN);
      subpatternWritingSlot=slot;subpatternOldSavePending=true;return;
    }
  }
  if(subpatternIndexBrowser!=patternBrowserRevision||subpatternIndexStorage!=patternStorageRevision
      ||subpatternIndexBusy||!subpatternIndexValid){serviceSubpatternIndex();return;}
  if(serviceSubpatternContentCounts())return;
  serviceSubpatternIndex();
}
FLASHMEM static void drawBrowserDeleteCross(int encoder) {
  int x1, center, x3;
  getIndicatorXPositions(encoder, x1, center, x3);
  ++x1; ++center; ++x3;
  light(x1, 3, UI_RED); light(x3, 3, UI_RED);
  light(center, 4, UI_RED);
  light(x1, 5, UI_RED); light(x3, 5, UI_RED);
}

FLASHMEM void openPatternConfirmation(unsigned target, bool deleting) {
  if (target < 1 || target > 999) return;
  patternConfirmReturnMode = currentMode;
  memcpy(patternConfirmReturnPos, currentMode->pos, sizeof(patternConfirmReturnPos));
  ++patternConfirmRevision;
  patternConfirmResource = 0;
  patternConfirmTarget = target;
  patternConfirmDelete = deleting;
  patternConfirmMode.pos[3] = 0; // NO is always the safe initial choice.
  const bool wasBypassed = bypassModeSwitchDebounce;
  bypassModeSwitchDebounce = true; switchMode(&patternConfirmMode);
  bypassModeSwitchDebounce = wasBypassed;
}

FLASHMEM void showPatternConfirmation() {
  FastLEDclear();
  const bool accept = patternConfirmMode.pos[3] == 1;
  const char *title = patternConfirmDelete ? "DEL?" : "SAVE?";
  if (maxX < 32 && !patternConfirmDelete) {
    // Compact S/E/? glyphs fit SAVE? starting at x=2 on a narrow panel.
    static const uint8_t glyphs[3][5] = {{3,2,3,1,3}, {3,2,3,2,3}, {3,1,1,0,1}};
    static const uint8_t starts[3] = {2,13,15};
    for (unsigned glyph = 0; glyph < 3; ++glyph)
      for (unsigned row = 0; row < 5; ++row)
        for (unsigned col = 0; col < 2; ++col)
          if (glyphs[glyph][row] & (2u >> col)) light(starts[glyph] + col, 15 - row, UI_RED);
    drawText("A", 5, 11, UI_RED); drawText("V", 9, 11, UI_RED);
  } else drawText(title, 2, 11, UI_RED);
  drawText(accept ? "YES" : "NO", 2, 5, accept ? UI_GREEN : UI_BLUE);
  drawIndicator('M', accept ? 'G' : 'X', 4, false, false);
  static int8_t lastChoice = -1;
  static uint32_t lastEntry = 0;
  const int8_t choice = accept ? 1 : 0;
  if (lastChoice != choice || lastEntry != patternConfirmRevision) {
    const uint32_t rgb = accept ? 0x00FF00 : 0x0000FF;
    Encoder[1].writeRGBCode(0x000000); Encoder[3].writeRGBCode(rgb);
    lastChoice = choice; lastEntry = patternConfirmRevision;
  }
  if (maxX >= 32 && patternConfirmResource != 1) drawNumber(patternConfirmTarget, UI_RED, 11);
  FastLEDshow();
}

FLASHMEM void finishPatternConfirmation(bool accept) {
  if (currentMode != &patternConfirmMode) return;
  for (int i = 0; i < NUM_ENCODERS; ++i) pressed[i] = false;
  if (!accept) showCanceledAnimation(false);
  else if (fastRecordActive || isRecording) showPatternResult(false);
  else {
    if (patternConfirmDelete && patternConfirmResource != 0) showPatternResult(deleteConfirmedBrowserResource());
    else if (patternConfirmDelete) deleteSelectedPatternFamily(patternConfirmTarget);
    else if (patternConfirmResource == 2) saveSamplePack(patternConfirmTarget);
    else saveSelectedPatternFamily(patternConfirmTarget, true);
  }
  const bool returnToBrowser = patternConfirmDelete || !accept || (patternConfirmResource == 0 && !lastPatternSaveOk);
  if (returnToBrowser && patternConfirmReturnMode) {
    Mode *browser = patternConfirmReturnMode;
    const bool wasBypassed = bypassModeSwitchDebounce;
    bypassModeSwitchDebounce = true; switchMode(browser);
    bypassModeSwitchDebounce = wasBypassed;
    memcpy(browser->pos, patternConfirmReturnPos, sizeof(patternConfirmReturnPos));
    if (browser == &loadSaveTrack) {
      browser->pos[2] = 0; SMP_LOAD_SETTINGS = true;
    }
    if (browser == &set_Wav) restoreSampleBrowserSelection(browser->pos[3]);
    else for (int i = 0; i < NUM_ENCODERS; ++i) Encoder[i].writeCounter((int32_t)browser->pos[i]);
  } else if (currentMode != &draw) switchMode(&draw);
  for (int i = 0; i < NUM_ENCODERS; ++i) pressed[i] = false;
  preventPaintUnpaint = true;
}

// Explicit family Save As/delete use the same cooperative I/O budget as staging.
// Nothing is installed until every incoming letter has a validated temporary file.
enum PatternFamilyPhase {
  FamilyIdle,FamilyConfirmScan,FamilySource,FamilyTargetCheck,FamilyRecoverRead,FamilyRecoverExists,
  FamilyRecoverRemove,FamilyRecoverRename,FamilyStage,FamilyStageRam,
  FamilySourceValidate,FamilyCopyOpenIn,FamilyCopyOpenOut,FamilyCopyTruncate,FamilyCopyRead,FamilyCopyWrite,
  FamilyCopyCloseIn,FamilyCopyCloseOut,FamilyCopyValidate,FamilyInstall,
  FamilyBakCheck,FamilyBakRemove,FamilyMoveOld,FamilyMoveNew,
  FamilyCleanupCheck,FamilyCleanupRemove,FamilyRollbackRemove,FamilyRollbackRestore,
  FamilyDeleteCheck,FamilyDeleteRemove,FamilyDeleteClear
};
static PatternFamilyPhase patternFamilyPhase=FamilyIdle;
static unsigned patternFamilyTarget=0,patternFamilySource=0;
static uint8_t patternFamilySlot=0,patternFamilyPart=0,patternFamilySourceMask=0,patternFamilyOldMask=0;
static uint8_t patternFamilyStaged=0,patternFamilyMoved=0,patternFamilyInstalled=0,patternFamilyCandidate=0;
static bool patternFamilyOverwrite=false,patternFamilySingle=false,patternFamilyFailed=false;
static SubpatternReader patternFamilyReader;
static SubpatternWriter patternFamilyWriter;
static Device patternFamilySettings;
static File patternFamilyIn,patternFamilyOut;
static uint8_t patternFamilyBuffer[512];
static size_t patternFamilyLeft=0,patternFamilyCount=0,patternFamilyWritten=0;
static unsigned patternFamilyClearCell=0;
static Note (*patternFamilyBlank)[maxY+1]=nullptr;

static bool patternFamilyBusy(){return patternFamilyPhase!=FamilyIdle;}
FLASHMEM static void patternFamilyPath(char *out,size_t size,unsigned file,uint8_t slot,uint8_t part=0) {
  char base[24];toern_pattern_slots::path(base,sizeof(base),file,slot);
  snprintf(out,size,part==0?"%s":part==1?"%s.tmp":"%s.bak",base);
}
FLASHMEM static void patternFamilyFinish(bool ok) {
  patternFamilyReader.cancel();patternFamilyWriter.cancel();
  if(patternFamilyIn)patternFamilyIn.close();if(patternFamilyOut)patternFamilyOut.close();
  patternFamilyPhase=FamilyIdle;lastPatternSaveOk=ok;++patternStorageRevision;
  if(ok){subpatternErrorUntil=0;subpatternSuccessUntil=millis()+700;}else subpatternFailure();
  preventPaintUnpaint=false;
}
FLASHMEM static void patternFamilyAbort() {
  patternFamilyReader.cancel();if(patternFamilyIn)patternFamilyIn.close();if(patternFamilyOut)patternFamilyOut.close();
  patternFamilySlot=0;patternFamilyPart=0;patternFamilyPhase=FamilyRollbackRemove;
}
FLASHMEM static void queuePatternFamilySave(unsigned target) {
  if(patternFamilyBusy()||subpatternWriteJob.busy()||subpatternWriteJob.incompleteTemporary())return;
  if(savedSubpatternMask(target)){openPatternConfirmation(target,false);return;}
  patternFamilyTarget=target;patternFamilySlot=0;patternFamilyPart=0;
  patternFamilyPhase=FamilyConfirmScan;
}
FLASHMEM static void beginPatternFamilySave(unsigned target,bool overwrite) {
  lastPatternSaveOk=false;
  if(patternFamilyBusy()||subpatternWriteJob.busy()||subpatternWriteJob.incompleteTemporary()||subpatternSelector||subpatternSwitchArmed||subpatternReady
      ||target<1||target>999||loadedPatternFile()<1||fastRecordActive||isRecording){subpatternFailure();return;}
  cancelSubpatternContentRead();
  patternFamilyTarget=target;patternFamilySource=loadedPatternFile();patternFamilyOverwrite=overwrite;
  subpatternCaptureSettings(patternFamilySettings,target);
  patternFamilySourceMask=1u<<subpatternSlot;patternFamilyOldMask=0;patternFamilyStaged=0;
  patternFamilyMoved=patternFamilyInstalled=0;patternFamilySlot=0;patternFamilyPart=0;patternFamilyFailed=false;
  patternFamilySingle=target==patternFamilySource;subpatternCancelPreview();
  if(patternFamilySingle) {
    char path[24];toern_pattern_slots::path(path,sizeof(path),target,subpatternSlot);
    patternFamilySlot=subpatternSlot;
    patternFamilyWriter.begin(path,note,patternFamilySettings,subpatternSlot,0,FILE_WRITE_BEGIN);
    patternFamilyPhase=FamilyStageRam;
  }else patternFamilyPhase=FamilySource;
}
FLASHMEM static void beginPatternFamilyDelete(unsigned target) {
  if(patternFamilyBusy()||subpatternWriteJob.busy()||subpatternWriteJob.incompleteTemporary()||subpatternReady||target<1||target>999||fastRecordActive||isRecording){subpatternFailure();return;}
  cancelSubpatternContentRead();subpatternCancelPreview();patternFamilyTarget=target;patternFamilySlot=1;patternFamilyPart=1;patternFamilyFailed=false;
  patternFamilyPhase=FamilyDeleteCheck;
}
FLASHMEM static void servicePatternFamilyJob() {
  if(!patternFamilyBusy()||fastRecordActive||isRecording)return;
  char path[40],other[40];
  const auto result=SubpatternIoResult::Running;
  (void)result;
  switch(patternFamilyPhase) {
    case FamilyConfirmScan:
      if(currentMode!=&loadSaveTrack){patternFamilyFinish(false);return;}
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyPart);
      if(SD.exists(path)){const unsigned target=patternFamilyTarget;patternFamilyPhase=FamilyIdle;openPatternConfirmation(target,false);return;}
      if(++patternFamilyPart==3){patternFamilyPart=0;if(++patternFamilySlot==5){const unsigned target=patternFamilyTarget;patternFamilyPhase=FamilyIdle;beginPatternFamilySave(target,false);}}return;
    case FamilySource:
      if(subpatternCache[patternFamilySlot]){patternFamilySourceMask|=1u<<patternFamilySlot;patternFamilyPart=0;++patternFamilySlot;}
      else {
        patternFamilyPath(path,sizeof(path),patternFamilySource,patternFamilySlot,patternFamilyPart);
        if(SD.exists(path)){patternFamilySourceMask|=1u<<patternFamilySlot;patternFamilyPart=0;++patternFamilySlot;}
        else if(++patternFamilyPart==3){patternFamilyPart=0;++patternFamilySlot;}
      }
      if(patternFamilySlot==5){patternFamilySlot=0;patternFamilyPart=0;patternFamilyPhase=FamilyTargetCheck;}return;
    case FamilyTargetCheck:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyPart);
      if(SD.exists(path)) {
        if(!patternFamilyOverwrite){patternFamilyFinish(false);return;}
        patternFamilyCandidate=0;patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);
        patternFamilyReader.begin(path,false);patternFamilyPhase=FamilyRecoverRead;return;
      }
      if(++patternFamilyPart==3){patternFamilyPart=0;if(++patternFamilySlot==5){patternFamilySlot=0;patternFamilyPhase=FamilyStage;}}
      return;
    case FamilyRecoverRead: {
      const auto r=patternFamilyReader.tick(SD);if(r==SubpatternIoResult::Running)return;
      if(r==SubpatternIoResult::Valid){
        if(patternFamilyCandidate==0){patternFamilyOldMask|=1u<<patternFamilySlot;
          patternFamilyPart=0;if(++patternFamilySlot==5){patternFamilySlot=0;patternFamilyPhase=FamilyStage;}else patternFamilyPhase=FamilyTargetCheck;
        }else patternFamilyPhase=FamilyRecoverExists;
        return;
      }
      if(++patternFamilyCandidate==3){patternFamilyFinish(false);return;}
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyCandidate);
      patternFamilyReader.begin(path,patternFamilyCandidate==1);return;
    }
    case FamilyRecoverExists:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);
      patternFamilyPhase=SD.exists(path)?FamilyRecoverRemove:FamilyRecoverRename;return;
    case FamilyRecoverRemove:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);
      if(!SD.remove(path)){patternFamilyFinish(false);return;}patternFamilyPhase=FamilyRecoverRename;return;
    case FamilyRecoverRename:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyCandidate);
      patternFamilyPath(other,sizeof(other),patternFamilyTarget,patternFamilySlot);
      if(!SD.rename(path,other)){patternFamilyFinish(false);return;}
      patternFamilyOldMask|=1u<<patternFamilySlot;patternFamilyPart=0;
      if(++patternFamilySlot==5){patternFamilySlot=0;patternFamilyPhase=FamilyStage;}else patternFamilyPhase=FamilyTargetCheck;return;
    case FamilyStage:
      if(patternFamilySlot==5){patternFamilySlot=0;patternFamilyPhase=FamilyInstall;return;}
      if(!(patternFamilySourceMask&(1u<<patternFamilySlot))){++patternFamilySlot;return;}
      patternFamilyStaged|=1u<<patternFamilySlot;
      if(subpatternCache[patternFamilySlot]) {
        patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);
        patternFamilyWriter.begin(path,subpatternCache[patternFamilySlot],patternFamilySettings,patternFamilySlot,0,FILE_WRITE_BEGIN,true);
        patternFamilyPhase=FamilyStageRam;
      }else {
        patternFamilyPath(path,sizeof(path),patternFamilySource,patternFamilySlot);
        patternFamilyCandidate=0;patternFamilyReader.begin(path,false);patternFamilyPhase=FamilySourceValidate;
      }return;
    case FamilyStageRam: {
      const auto r=patternFamilyWriter.tick(SD);if(r==SubpatternIoResult::Running)return;
      if(patternFamilySingle) {
        if(r==SubpatternIoResult::Valid){subpatternDirtyMask&=~(1u<<patternFamilySlot);deletedSubpatternSlots&=~(1u<<patternFamilySlot);subpatternEmptyMask&=~(1u<<patternFamilySlot);subpatternAvailable|=1u<<patternFamilySlot;}
        patternFamilyFinish(r==SubpatternIoResult::Valid);return;
      }
      if(r!=SubpatternIoResult::Valid){patternFamilyAbort();return;}
      ++patternFamilySlot;patternFamilyPhase=FamilyStage;return;
    }
    case FamilySourceValidate: {
      const auto r=patternFamilyReader.tick(SD);if(r==SubpatternIoResult::Running)return;
      if(r!=SubpatternIoResult::Valid){
        if(++patternFamilyCandidate==3){patternFamilyAbort();return;}
        patternFamilyPath(path,sizeof(path),patternFamilySource,patternFamilySlot,patternFamilyCandidate);
        patternFamilyReader.begin(path,patternFamilyCandidate==1);return;
      }
      patternFamilyPhase=FamilyCopyOpenIn;return;
    }
    case FamilyCopyOpenIn:
      patternFamilyPath(path,sizeof(path),patternFamilySource,patternFamilySlot,patternFamilyCandidate);patternFamilyIn=SD.open(path);
      if(!patternFamilyIn){patternFamilyAbort();return;}patternFamilyLeft=patternFamilyIn.size();patternFamilyPhase=FamilyCopyOpenOut;return;
    case FamilyCopyOpenOut:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,1);patternFamilyOut=SD.open(path,FILE_WRITE_BEGIN);
      if(!patternFamilyOut){patternFamilyAbort();return;}patternFamilyPhase=FamilyCopyTruncate;return;
    case FamilyCopyTruncate:
      if(!patternFamilyOut.truncate(0)){patternFamilyAbort();return;}patternFamilyPhase=FamilyCopyRead;return;
    case FamilyCopyRead:
      if(!patternFamilyLeft){patternFamilyPhase=FamilyCopyCloseIn;return;}
      patternFamilyCount=std::min(patternFamilyLeft,sizeof(patternFamilyBuffer));patternFamilyWritten=0;
      if((size_t)patternFamilyIn.read(patternFamilyBuffer,patternFamilyCount)!=patternFamilyCount){patternFamilyAbort();return;}
      patternFamilyPhase=FamilyCopyWrite;return;
    case FamilyCopyWrite: {
      const size_t n=patternFamilyOut.write(patternFamilyBuffer+patternFamilyWritten,patternFamilyCount-patternFamilyWritten);
      if(!n||n>patternFamilyCount-patternFamilyWritten){patternFamilyAbort();return;}patternFamilyWritten+=n;
      if(patternFamilyWritten==patternFamilyCount){patternFamilyLeft-=patternFamilyCount;patternFamilyPhase=FamilyCopyRead;}return;
    }
    case FamilyCopyCloseIn:patternFamilyIn.close();patternFamilyPhase=FamilyCopyCloseOut;return;
    case FamilyCopyCloseOut:
      patternFamilyOut.close();patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,1);
      patternFamilyReader.begin(path,false);patternFamilyPhase=FamilyCopyValidate;return;
    case FamilyCopyValidate: {
      const auto r=patternFamilyReader.tick(SD);if(r==SubpatternIoResult::Running)return;
      if(r!=SubpatternIoResult::Valid){patternFamilyAbort();return;}++patternFamilySlot;patternFamilyPhase=FamilyStage;return;
    }
    case FamilyInstall:
      if(patternFamilySlot==5){patternFamilySlot=0;patternFamilyPart=0;patternFamilyPhase=FamilyCleanupCheck;return;}
      if(patternFamilyOldMask&(1u<<patternFamilySlot)){patternFamilyPhase=FamilyBakCheck;return;}
      if(patternFamilySourceMask&(1u<<patternFamilySlot)){patternFamilyPhase=FamilyMoveNew;return;}
      ++patternFamilySlot;return;
    case FamilyBakCheck:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,2);
      patternFamilyPhase=SD.exists(path)?FamilyBakRemove:FamilyMoveOld;return;
    case FamilyBakRemove:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,2);
      if(!SD.remove(path)){patternFamilyAbort();return;}patternFamilyPhase=FamilyMoveOld;return;
    case FamilyMoveOld:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);
      patternFamilyPath(other,sizeof(other),patternFamilyTarget,patternFamilySlot,2);
      if(!SD.rename(path,other)){patternFamilyAbort();return;}patternFamilyMoved|=1u<<patternFamilySlot;
      if(patternFamilySourceMask&(1u<<patternFamilySlot))patternFamilyPhase=FamilyMoveNew;
      else{++patternFamilySlot;patternFamilyPhase=FamilyInstall;}return;
    case FamilyMoveNew:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,1);
      patternFamilyPath(other,sizeof(other),patternFamilyTarget,patternFamilySlot);
      if(!SD.rename(path,other)){patternFamilyAbort();return;}
      patternFamilyInstalled|=1u<<patternFamilySlot;++patternFamilySlot;patternFamilyPhase=FamilyInstall;return;
    case FamilyCleanupCheck:
      if(patternFamilySlot==5){
        subpatternFile=SMP.file=patternFamilyTarget;subpatternDirtyMask=0;deletedSubpatternSlots=0;subpatternEmptyMask=0;subpatternAvailable=patternFamilySourceMask;
        if(currentMode!=&draw)switchMode(&draw);patternFamilyFinish(true);return;
      }
      if(patternFamilySourceMask&(1u<<patternFamilySlot)){++patternFamilySlot;patternFamilyPart=0;return;}
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyPart);
      if(SD.exists(path)){patternFamilyPhase=FamilyCleanupRemove;return;}
      if(++patternFamilyPart==3){patternFamilyPart=0;++patternFamilySlot;}return;
    case FamilyCleanupRemove:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,patternFamilyPart);
      if(!SD.remove(path)){patternFamilyFinish(false);return;}
      if(++patternFamilyPart==3){patternFamilyPart=0;++patternFamilySlot;}patternFamilyPhase=FamilyCleanupCheck;return;
    case FamilyRollbackRemove:
      if(patternFamilySlot==5){patternFamilyFinish(false);return;}
      if(patternFamilyPart==0&&(patternFamilyInstalled&(1u<<patternFamilySlot))){patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot);SD.remove(path);patternFamilyPart=1;return;}
      if(patternFamilyPart<2&&(patternFamilyStaged&(1u<<patternFamilySlot))){patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,1);SD.remove(path);patternFamilyPart=2;return;}
      patternFamilyPhase=FamilyRollbackRestore;return;
    case FamilyRollbackRestore:
      if(patternFamilyMoved&(1u<<patternFamilySlot)){
        patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot,2);
        patternFamilyPath(other,sizeof(other),patternFamilyTarget,patternFamilySlot);SD.rename(path,other);
      }
      ++patternFamilySlot;patternFamilyPart=0;patternFamilyPhase=FamilyRollbackRemove;return;
    case FamilyDeleteCheck: {
      if(patternFamilySlot==5&&patternFamilyFailed){patternFamilyFinish(false);return;}
      if(patternFamilySlot==6){
        if(patternFamilyFailed){patternFamilyFinish(false);return;}
        if(patternFamilyTarget!=loadedPatternFile()){patternFamilyFinish(!patternFamilyFailed);return;}
        patternFamilyBlank=subpatternFreeBank();if(!patternFamilyBlank){patternFamilyFinish(false);return;}
        patternFamilyClearCell=0;patternFamilyPhase=FamilyDeleteClear;return;
      }
      const uint8_t slot=patternFamilySlot%5;
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,slot,patternFamilyPart%3);
      if(SD.exists(path)){patternFamilyPhase=FamilyDeleteRemove;return;}
      if(++patternFamilyPart==4){patternFamilyPart=1;++patternFamilySlot;}return;
    }
    case FamilyDeleteRemove:
      patternFamilyPath(path,sizeof(path),patternFamilyTarget,patternFamilySlot%5,patternFamilyPart%3);
      if(!SD.remove(path))patternFamilyFailed=true;
      if(++patternFamilyPart==4){patternFamilyPart=1;++patternFamilySlot;}patternFamilyPhase=FamilyDeleteCheck;return;
    case FamilyDeleteClear: {
      const unsigned end=(maxlen+1)*(maxY+1);
      for(;patternFamilyClearCell<end;++patternFamilyClearCell)
        patternFamilyBlank[patternFamilyClearCell/(maxY+1)][patternFamilyClearCell%(maxY+1)]={0,defaultVelocity,100,1,NOTE_MIDI_PITCH_NONE};
      if(patternFamilyClearCell==(maxlen+1)*(maxY+1)) {
        {toern_audio::StateGuard guard;note=patternFamilyBlank;}
        updateLastPage();subpatternNotePagesLoaded();deletedSubpatternSlots=0x1F;patternFamilyFinish(!patternFamilyFailed);
      }return;
    }
    default:return;
  }
}
