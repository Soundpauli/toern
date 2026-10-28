#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>

// Cooperative pattern I/O. A tick performs at most one 512-byte read/write or
// one filesystem transition. Callers must return to their event loop between ticks.
namespace toern_pattern_io {
enum class Result : uint8_t { Running, Valid, Missing, Invalid, Failed };

template<class File, class Note, class Settings, unsigned Steps, unsigned Rows>
class Reader {
public:
  static constexpr size_t cells = Steps * Rows;
  static constexpr size_t notesBytes = cells * 4;
  static constexpr size_t baseBytes = notesBytes + 2 + sizeof(Settings);
  static constexpr size_t pitchEnd = baseBytes + 8 + cells;
  void begin(const char *path, bool requirePitch, Note (*grid)[Rows+1] = nullptr) {
    cancel(); snprintf(path_, sizeof(path_), "%s", path);
    required_ = requirePitch; grid_ = grid; state_ = Open; result_ = Result::Running;
    offset_ = 0; end_ = baseBytes; pitch_ = false; hasNotes_ = false; memset(header_, 0, 8);
  }
  void cancel() { if (file_) file_.close(); state_=Idle; result_=Result::Missing; }
  Result result() const { return result_; }
  bool busy() const { return result_ == Result::Running; }
  bool hasNotes()const{return hasNotes_;}
  template<class FS> Result tick(FS &fs) {
    if (!busy()) return result_;
    if (state_ == Open) {
      file_=fs.open(path_);
      if (!file_) { result_=Result::Missing; state_=Idle; return result_; }
      size_=file_.size();
      if (size_ < baseBytes || (required_ && size_ < pitchEnd)) return finish(Result::Invalid);
      end_=size_ >= baseBytes+8 ? baseBytes+8 : baseBytes;
      state_=Read; return result_;
    }
    if (state_ == Close) { file_.close(); state_=Idle; result_=pending_; return result_; }
    const size_t n=std::min(sizeof(buffer_), end_-offset_);
    if ((size_t)file_.read(buffer_,n)!=n) return finish(Result::Invalid);
    for (size_t i=0;i<n;++i) {
      const size_t at=offset_+i; const uint8_t v=buffer_[i];
      if(at<notesBytes&&at%4==0&&v>=1&&v<=16)hasNotes_=true;
      if (at<notesBytes && grid_) {
        const size_t cell=at/4; Note &dst=grid_[1+cell/Rows][1+cell%Rows];
        switch(at%4) {
          case 0: dst.channel=v<=16?v:0; dst.midiPitch=255; break;
          case 1: dst.velocity=std::min(v,(uint8_t)127); break;
          case 2: dst.probability=std::min(v,(uint8_t)100); break;
          case 3: dst.condition=v; break;
        }
      } else if (at==notesBytes && v!=0xFF) return finish(Result::Invalid);
      else if (at==notesBytes+1 && v!=0xFE) return finish(Result::Invalid);
      if (at>=baseBytes && at<baseBytes+8) header_[at-baseBytes]=v;
      if (pitch_ && at>=baseBytes+8 && at<pitchEnd && grid_) {
        const size_t cell=at-baseBytes-8;
        grid_[1+cell/Rows][1+cell%Rows].midiPitch=v<=127?v:255;
      }
    }
    offset_+=n;
    if (offset_==baseBytes+8) {
      static const uint8_t expected[8]={'T','P','I','T',1,0x10,0,0};
      pitch_=memcmp(header_,expected,8)==0;
      if (pitch_) { if(size_<pitchEnd)return finish(Result::Invalid); end_=pitchEnd; }
      else if(required_) return finish(Result::Invalid);
    }
    if(offset_==end_) return finish(required_&&!pitch_?Result::Invalid:Result::Valid);
    return result_;
  }
private:
  Result finish(Result value) { pending_=value; state_=Close; return Result::Running; }
  enum State { Idle, Open, Read, Close } state_=Idle;
  File file_; char path_[40]{}; uint8_t buffer_[512]{},header_[8]{};
  Note (*grid_)[Rows+1]=nullptr;
  size_t offset_=0,end_=0,size_=0; bool required_=false,pitch_=false,hasNotes_=false;
  Result result_=Result::Missing,pending_=Result::Missing;
};

template<class File, class Note, class Settings, unsigned Steps, unsigned Rows>
class Writer {
public:
  using ReadJob=Reader<File,Note,Settings,Steps,Rows>;
  static constexpr size_t totalBytes=ReadJob::pitchEnd+8;
  void begin(const char *path, const Note (*grid)[Rows+1], const Settings &settings,
             uint8_t slot, uint8_t deleted, int flags, bool stageOnly=false) {
    const bool discardOwned=ownsTemporary_ && offset_<totalBytes && strcmp(path_,path)==0;
    cancel(); snprintf(path_,sizeof(path_),"%s",path);
    snprintf(tmp_,sizeof(tmp_),"%s.tmp",path); snprintf(bak_,sizeof(bak_),"%s.bak",path);
    grid_=grid; memcpy(&settings_,&settings,sizeof(settings_)); flags_=flags; stageOnly_=stageOnly;
    const uint8_t tail[8]={'T','S','U','B',1,slot,0,deleted}; memcpy(identity_,tail,8);
    offset_=0; candidate_=0; ownsTemporary_=discardOwned; moved_=false; sawInvalid_=false; state_=discardOwned?OwnTempExists:Check; result_=Result::Running;
    reader_.begin(path_,false);
  }
  void cancel() { reader_.cancel(); if(file_)file_.close(); state_=Idle; result_=Result::Missing; }
  bool incompleteTemporary()const{return ownsTemporary_&&offset_<totalBytes;}
  template<class FS> bool abandon(FS &fs) {
    const bool incomplete=incompleteTemporary();
    cancel();
    if(incomplete&&fs.remove(tmp_))ownsTemporary_=false;
    return !incompleteTemporary();
  }
  bool busy()const{return result_==Result::Running;}
  Result result()const{return result_;}
  template<class FS> Result tick(FS &fs) {
    if(!busy())return result_;
    switch(state_) {
      case OwnTempExists: state_=fs.exists(tmp_)?OwnTempRemove:Check;if(state_==Check)ownsTemporary_=false;return result_;
      case OwnTempRemove: if(!fs.remove(tmp_))return fail();ownsTemporary_=false;state_=Check;return result_;
      case Check: {
        const Result r=reader_.tick(fs); if(r==Result::Running)return result_;
        if(r==Result::Valid) {
          if(candidate_==0){state_=Open;return result_;}
          state_=CheckRemove;return result_;
        }
        sawInvalid_|=r!=Result::Missing;
        if(++candidate_<3){reader_.begin(candidate_==1?tmp_:bak_,candidate_==1);return result_;}
        if(sawInvalid_)return fail();
        state_=Open;return result_;
      }
      case CheckRemove: exists_=fs.exists(path_);state_=exists_?Remove:Recover;return result_;
      case Remove: if(!fs.remove(path_))return fail();state_=Recover;return result_;
      case Recover: if(!fs.rename(candidate_==1?tmp_:bak_,path_))return fail();state_=Open;return result_;
      case Open:
        file_=fs.open(tmp_,flags_);if(!file_)return fail();ownsTemporary_=true;state_=Truncate;return result_;
      case Truncate: if(!file_.truncate(0)){state_=FailedClose;return result_;}state_=Write;return result_;
      case Write: {
        const size_t n=std::min(sizeof(buffer_),totalBytes-offset_);
        for(size_t i=0;i<n;++i)buffer_[i]=byteAt(offset_+i);
        const size_t nWritten=file_.write(buffer_,n);if(!nWritten||nWritten>n){state_=FailedClose;return result_;}
        offset_+=nWritten;if(offset_==totalBytes)state_=Close;return result_;
      }
      case Close: file_.close();reader_.begin(tmp_,true);state_=Verify;return result_;
      case Verify: {
        const Result r=reader_.tick(fs);if(r==Result::Running)return result_;
        if(r!=Result::Valid)return fail();if(stageOnly_)return success();state_=DestExists;return result_;
      }
      case DestExists: exists_=fs.exists(path_);state_=exists_?BakExists:Install;return result_;
      case BakExists: exists_=fs.exists(bak_);state_=exists_?RemoveBak:MoveOld;return result_;
      case RemoveBak: if(!fs.remove(bak_))return fail();state_=MoveOld;return result_;
      case MoveOld: if(!fs.rename(path_,bak_))return fail();moved_=true;state_=Install;return result_;
      case Install:
        if(!fs.rename(tmp_,path_)){if(moved_){state_=Rollback;return result_;}return fail();}
        ownsTemporary_=false;reader_.begin(path_,true);state_=VerifyFinal;return result_;
      case VerifyFinal: {
        const Result r=reader_.tick(fs);if(r==Result::Running)return result_;
        return r==Result::Valid?success():fail();
      }
      case Rollback: fs.rename(bak_,path_);return fail();
      case FailedClose: file_.close();state_=FailedCleanup;return result_;
      case FailedCleanup: if(fs.remove(tmp_))ownsTemporary_=false;return fail();
      default:return fail();
    }
  }
private:
  uint8_t byteAt(size_t at)const {
    if(at<ReadJob::notesBytes){const size_t cell=at/4;const Note &n=grid_[1+cell/Rows][1+cell%Rows];
      switch(at%4){case 0:return n.channel;case 1:return n.velocity;case 2:return n.probability;default:return n.condition;}}
    if(at==ReadJob::notesBytes)return 0xFF;if(at==ReadJob::notesBytes+1)return 0xFE;
    if(at<ReadJob::baseBytes)return ((const uint8_t*)&settings_)[at-ReadJob::notesBytes-2];
    static const uint8_t header[8]={'T','P','I','T',1,0x10,0,0};
    if(at<ReadJob::baseBytes+8)return header[at-ReadJob::baseBytes];
    if(at<ReadJob::pitchEnd){const size_t cell=at-ReadJob::baseBytes-8;return grid_[1+cell/Rows][1+cell%Rows].midiPitch;}
    return identity_[at-ReadJob::pitchEnd];
  }
  Result fail(){state_=Idle;result_=Result::Failed;return result_;}
  Result success(){state_=Idle;result_=Result::Valid;return result_;}
  enum State{Idle,OwnTempExists,OwnTempRemove,Check,CheckRemove,Remove,Recover,Open,Truncate,Write,Close,Verify,DestExists,BakExists,RemoveBak,MoveOld,Install,VerifyFinal,Rollback,FailedClose,FailedCleanup}state_=Idle;
  File file_;ReadJob reader_;Settings settings_{};const Note (*grid_)[Rows+1]=nullptr;
  char path_[32]{},tmp_[40]{},bak_[40]{};uint8_t buffer_[512]{},identity_[8]{};
  size_t offset_=0;int flags_=0;uint8_t candidate_=0;bool moved_=false,exists_=false,stageOnly_=false,sawInvalid_=false,ownsTemporary_=false;
  Result result_=Result::Missing;
};
}
