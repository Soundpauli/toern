#pragma once
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Cooperative KIT export. Each tick performs at most one filesystem operation
// or one bounded 2 KB read/write. Audio/MIDI IRQs are never masked here.
namespace toern_kit_io {
constexpr size_t transferBytes=2048;
enum class Result : uint8_t { Idle, Running, Saved, Failed, Cancelled };
struct Source { const int16_t *pcm=nullptr; uint32_t samples=0; };

template<class File> class Writer {
public:
  // Only this raw sector buffer may live in Teensy's NOLOAD DMA memory.
  // File objects and state require ordinary C++ startup initialization.
  explicit Writer(uint8_t (&sector)[transferBytes]) : buffer_(sector) {}
  bool begin(unsigned target, const Source (&sources)[8], int writeFlags) {
    if(busy()||target>999)return false;
    target_=target;memcpy(sources_,sources,sizeof(sources_));flags_=writeFlags;progress_=0;
    for(const auto &source:sources)if(source.samples&&!source.pcm)return false;
    slot_=0;staged_=installed_=backed_=0;cancel_=committed_=rollbackFailed_=false;
    result_=Result::Running;state_=DirExists;return true;
  }
  bool busy()const{return result_==Result::Running;}
  Result result()const{return result_;}
  void cancel(){cancel_=true;}
  unsigned progress()const {return progress_;}
  template<class FS> Result tick(FS &fs) {
    if(!busy())return result_;
    if(cancel_&&!committed_&&state_<CloseAbortIn) {failure_=Result::Cancelled;state_=CloseAbortIn;}
    char dest[32],tmp[40],bak[40];paths(dest,tmp,bak);
    switch(state_) {
      case DirExists: {char dir[12];snprintf(dir,sizeof(dir),"%u",target_);state_=fs.exists(dir)?Start:DirMake;break;}
      case DirMake: {char dir[12];snprintf(dir,sizeof(dir),"%u",target_);if(!fs.mkdir(dir))return fail();state_=Start;break;}
      case Start:
        offset_=0;total_=0;crc_=verifyCrc_=0;state_=TempExists;break;
      case TempExists: state_=fs.exists(tmp)?TempRemove:OutOpen;break;
      case TempRemove: if(!fs.remove(tmp))return fail();state_=OutOpen;break;
      case OutOpen: out_=fs.open(tmp,flags_);if(!out_)return fail();staged_|=1u<<slot_;state_=Truncate;break;
      case Truncate:
        if(!out_.truncate(0))return fail();
        if(sources_[slot_].samples>0x7FFFFFD9u)return fail();
        total_=sources_[slot_].samples?44u+sources_[slot_].samples*2u:0;
        state_=total_?Header:CloseIn;break;
      case Header:
        header(buffer_,total_-44u);
        if(out_.write(buffer_,44)!=44)return fail();
        crc_=crc(crc_,buffer_,44);offset_=44;state_=offset_==total_?CloseIn:Write;break;
      case Write:
        count_=std::min((uint32_t)transferBytes,total_-offset_);
        memcpy(buffer_,reinterpret_cast<const uint8_t*>(sources_[slot_].pcm)+(offset_-44u),count_);
        if(out_.write(buffer_,count_)!=count_)return fail();
        crc_=crc(crc_,buffer_,count_);offset_+=count_;
        progress_=slot_*11u+(unsigned)((uint64_t)offset_*5u/total_);
        state_=offset_==total_?CloseIn:Write;break;
      case CloseIn: if(in_)in_.close();state_=CloseOut;break;
      case CloseOut: out_.close();state_=VerifyOpen;break;
      case VerifyOpen:
        in_=fs.open(tmp);if(!in_||in_.size()!=total_)return fail();
        offset_=0;state_=total_?VerifyRead:VerifyClose;break;
      case VerifyRead:
        count_=std::min((uint32_t)transferBytes,total_-offset_);
        if((size_t)in_.read(buffer_,count_)!=count_)return fail();
        verifyCrc_=crc(verifyCrc_,buffer_,count_);offset_+=count_;
        progress_=slot_*11u+5u+(unsigned)((uint64_t)offset_*6u/total_);
        if(offset_==total_)state_=VerifyClose;break;
      case VerifyClose:
        in_.close();if(verifyCrc_!=crc_)return fail();
        progress_=(slot_+1u)*11u;
        if(++slot_<8)state_=Start;else {slot_=0;state_=BackupExists;}break;
      // Stage and verify all eight slots before replacing any primary.
      case BackupExists: state_=fs.exists(bak)?PrimaryRecoveryCheck:PrimaryExists;break;
      case PrimaryRecoveryCheck: state_=fs.exists(dest)?BackupRemove:Recover;break;
      case Recover: if(!fs.rename(bak,dest))return fail();state_=PrimaryExists;break;
      case BackupRemove: if(!fs.remove(bak))return fail();state_=PrimaryExists;break;
      case PrimaryExists: state_=fs.exists(dest)?BackupMove:Install;break;
      case BackupMove: if(!fs.rename(dest,bak))return fail();backed_|=1u<<slot_;state_=Install;break;
      case Install:
        if(!fs.rename(tmp,dest))return fail();installed_|=1u<<slot_;staged_&=~(1u<<slot_);
        progress_=89u+slot_;
        if(++slot_<8)state_=BackupExists;else {committed_=true;slot_=0;state_=CleanupBackup;}break;
      case CleanupBackup:
        // New primaries are committed. A failed cleanup may leave a harmless
        // prior generation; it must not turn an installed KIT into a failure.
        if(backed_&(1u<<slot_))fs.remove(bak);
        if(++slot_==8){progress_=100;result_=Result::Saved;state_=Idle;}break;
      case CloseAbortIn: if(in_)in_.close();state_=CloseAbortOut;break;
      case CloseAbortOut: if(out_)out_.close();slot_=0;state_=RollbackRemove;break;
      case RollbackRemove:
        if(installed_&(1u<<slot_)) {
          if(!fs.remove(dest)){rollbackFailed_=true;backed_&=~(1u<<slot_);}
        }
        state_=RollbackRestore;break;
      case RollbackRestore:
        if(backed_&(1u<<slot_))if(!fs.rename(bak,dest))rollbackFailed_=true;
        state_=RollbackTemp;break;
      case RollbackTemp:
        if(staged_&(1u<<slot_))fs.remove(tmp);
        if(++slot_==8){result_=rollbackFailed_?Result::Failed:failure_;state_=Idle;}
        else state_=RollbackRemove;break;
      default: break;
    }
    return result_;
  }
private:
  Result fail(){failure_=Result::Failed;state_=CloseAbortIn;rollbackFailed_=false;return result_;}
  void paths(char *dest,char *tmp,char *bak)const {
    snprintf(dest,32,"%u/%u.wav",target_,slot_+1);
    snprintf(tmp,40,"%s.kit-tmp",dest);snprintf(bak,40,"%s.kit-bak",dest);
  }
  static uint32_t crc(uint32_t value,const uint8_t *data,size_t size) {
    value=~value;
    while(size--){value^=*data++;for(unsigned bit=0;bit<8;++bit)value=(value>>1)^((value&1)?0xEDB88320u:0);}
    return ~value;
  }
  static void header(uint8_t *out,uint32_t bytes) {
    static const uint8_t base[44]={'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
      1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    memcpy(out,base,44);const uint32_t riff=bytes+36;memcpy(out+4,&riff,4);memcpy(out+40,&bytes,4);
  }
  enum State {Idle,DirExists,DirMake,Start,TempExists,TempRemove,OutOpen,Truncate,
    Header,Write,CloseIn,CloseOut,VerifyOpen,VerifyRead,VerifyClose,BackupExists,PrimaryRecoveryCheck,
    Recover,BackupRemove,PrimaryExists,BackupMove,Install,CleanupBackup,
    CloseAbortIn,CloseAbortOut,RollbackRemove,RollbackRestore,RollbackTemp} state_=Idle;
  Source sources_[8]{};File in_,out_;
  uint8_t *const buffer_;
  unsigned target_=0,slot_=0,progress_=0;int flags_=0;
  uint32_t total_=0,offset_=0,count_=0,crc_=0,verifyCrc_=0;
  uint8_t staged_=0,installed_=0,backed_=0;
  bool cancel_=false,committed_=false,rollbackFailed_=false;
  Result result_=Result::Idle,failure_=Result::Failed;
};
} // namespace toern_kit_io
