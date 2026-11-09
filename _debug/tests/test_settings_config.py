"""Exercise the USB settings endpoint with host EEPROM, audio and serial mocks."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
menu = (root / 'platformio/toern_menu.ino').read_text()
def function(name):
    match = re.search(r'^(?:FLASHMEM )?(?:static )?(?:inline )?[^\n]*\b' + name + r'\([^\n]*\) \{', menu, re.M)
    start = match.start(); end = menu.index('{', start) + 1; depth = 1
    while depth:
        depth += (menu[end] == '{') - (menu[end] == '}'); end += 1
    return menu[start:end]
fixture = subprocess.check_output(['node', '--input-type=module', '-e', "import {makeSettings,DEFAULT_ANSWERS} from './tools/firmware/setup-settings.js'; const b=new Uint8Array(60);b[13]=4;console.log(Array.from(makeSettings(DEFAULT_ANSWERS,b)).join(','));"], cwd=root, text=True).strip()
code = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#define FLASHMEM
constexpr int EEPROM_DATA_START=43,SETTINGS_EEPROM_BLOCK_LEN=60;
struct SerialMock {
 std::string output;
 void printf(const char *format,...){char buffer[512];va_list args;va_start(args,format);vsnprintf(buffer,sizeof(buffer),format,args);va_end(args);output+=buffer;}
 void println(const char *line){output+=line;output+='\n';}
} Serial;
struct Eeprom {
 uint8_t data[256]{};int writes=0;
 uint8_t read(int i){return data[i];}
 void write(int i,uint8_t b){data[i]=b;++writes;}
} EEPROM;
struct Mode{} draw;
bool isRecording=false,fastRecordActive=false,isNowPlaying=false,quiet=true,settingsBackupDirty=false,bypassModeSwitchDebounce=false;
bool applied=false,reloaded=false,backupWorks=true;
unsigned int pendingPage=9,lastFlowPage=4;
int refreshed=0,mutesApplied=0,activity=0;
void unmuteAllChannels(){}
void applyMutesAfterPMODSwitch(){++mutesApplied;}
void refreshCtrlEncoderConfig(){++refreshed;}
void noteUserActivity(){++activity;}
int micGain=0,previewVol=0,previewTriggerMode=0,lineOutLevelSetting=0,recInput=0;
struct {int vol=0;} GLOB;
struct {void inputSelect(int){}} sgtl5000_1;
#define VERSION "test"
void pause(bool){isNowPlaying=false;}
void allOff(){}void stopAllSetWavPreviewAudio(){}
bool sdIoAudioSettled(){return quiet;}
void serviceSettingsBackup(){}
void setTouchyEnabled(bool){}
void loadMenuFromEEPROM(){reloaded=true;}
void loadColorSchemeAndBrightness(){}
void applyAudioSettingsFromGlobals(){applied=true;}
void resetMidiClockState(){}
void markSettingsBackupDirty(){settingsBackupDirty=true;}
void flushSettingsBackupNow(){if(backupWorks)settingsBackupDirty=false;}
void switchMode(Mode *m){assert(bypassModeSwitchDebounce&&m==&draw);}
HELPERS
#include "toern_settings_serial.ino"
uint8_t validBlock[60]={FIXTURE};
void request(std::string s){std::vector<char> args(s.begin(),s.end());args.push_back(0);Serial.output.clear();handleSettingsConfigCommand(args.data());}
std::string set(uint8_t *bytes){char cmd[160],hex[121];encodeHexBytes(bytes,60,hex,sizeof(hex));snprintf(cmd,sizeof(cmd),"SET %08X %s",crc32_compute(EEPROM.data+43,60),hex);return cmd;}
void error(const char *s){assert(Serial.output.find(std::string("\"error\":\"")+s+"\"")!=std::string::npos);}
int main(){
 memcpy(EEPROM.data+43,validBlock,60);EEPROM.data[0]=77;EEPROM.data[201]=1;
 request("GET");assert(Serial.output.find("\"protocol\":1")!=std::string::npos&&EEPROM.writes==0);
 uint8_t desired[60];memcpy(desired,validBlock,60);desired[27]=0;
 isNowPlaying=true;request(set(desired));error("BUSY");assert(EEPROM.writes==0);
 request("STOP");assert(!isNowPlaying&&EEPROM.writes==0);
 quiet=false;request(set(desired));error("BUSY");assert(EEPROM.writes==0);quiet=true;
 isRecording=true;request("STOP");error("RECORDING");request(set(desired));error("BUSY");isRecording=false;
 request("SET 00000000 "+std::string(120,'0'));error("CHANGED");assert(EEPROM.writes==0);
 desired[13]=1;request(set(desired));error("LAYOUT");assert(EEPROM.writes==0);desired[13]=4;
 desired[41]=255;request(set(desired));error("VALUES");assert(EEPROM.writes==0);desired[41]=10;
 request("SET 0000000X "+std::string(120,'0'));error("FORMAT");
 request("SET 00000000 0");error("FORMAT");
 request(set(desired));assert(EEPROM.writes==60&&applied&&reloaded&&refreshed==1&&mutesApplied==1&&activity==1);assert(pendingPage==0&&lastFlowPage==0);
 assert(!memcmp(EEPROM.data+43,desired,60));assert(EEPROM.data[0]==77&&EEPROM.data[201]==1);
 assert(Serial.output.find("\"backup\":true")!=std::string::npos&&GLOB.vol==60&&previewVol==20&&lineOutLevelSetting==30);
 backupWorks=false;request(set(validBlock));assert(Serial.output.find("\"backup\":false")!=std::string::npos);
}
'''.replace('HELPERS','\n'.join(function(n) for n in ['crc32_update','crc32_compute','hexNibble','decodeHexBytes','encodeHexBytes'])).replace('FIXTURE',fixture)
with tempfile.TemporaryDirectory() as tmp:
    src, exe = Path(tmp)/'config.cpp',Path(tmp)/'config';src.write_text(code)
    flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
    subprocess.run(['c++','-std=c++17',*flags,'-I',str(root/'platformio'),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: CONFIG read, audio guards, recording guard, stale CRC, malformed/invalid data, physical layout, complete persistence and SD backup status.')
