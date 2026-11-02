"""Injected NACKs/short reads through the actual patched DuPPa driver."""
from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
lib=root/'.pio/libdeps/teensy41/DuPPa Library/src'
assert 'const uint8_t failure=' in (lib/'i2cEncoderLibV2.cpp').read_text(),'Run pio run to apply dependency patches'
with tempfile.TemporaryDirectory(prefix='toern-encoder-failures-') as tmp:
 p=Path(tmp)
 (p/'Arduino.h').write_text('#pragma once\n#include <stdint.h>\n#include <cstddef>\ninline void delay(unsigned){}\n')
 (p/'Wire.h').write_text(r'''
#pragma once
#include <stdint.h>
#include <vector>
struct TwoWire {
 int error=0,requests=0,at=0;uint8_t requested=0;std::vector<int> data;
 void beginTransmission(uint8_t){at=0;}void write(uint8_t){}void write(uint8_t*,uint8_t){}
 uint8_t endTransmission(){return error;}
 uint8_t requestFrom(uint8_t,uint8_t count){requests++;requested=count;return data.size();}
 int available(){return data.size()-at;}int read(){return at<(int)data.size()?data[at++]:-1;}
};extern TwoWire Wire;
''')
 (p/'test.cpp').write_text(r'''
#include <cassert>
#include <cstdio>
#include <Wire.h>
#include "i2cEncoderLibV2.h"
TwoWire Wire;int callbacks=0;
void callback(i2cEncoderLibV2*){callbacks++;}
int main(){
 i2cEncoderLibV2 encoder(0x30);
 encoder.onButtonRelease=encoder.onButtonPush=encoder.onButtonDoublePush=callback;
 encoder.onIncrement=encoder.onDecrement=encoder.onChange=callback;
 encoder.onMax=encoder.onMinMax=encoder.onMin=callback;
 encoder.onGP1Rise=encoder.onGP1Fall=encoder.onGP2Rise=encoder.onGP2Fall=encoder.onGP3Rise=encoder.onGP3Fall=encoder.onFadeProcess=callback;
 Wire.error=2;Wire.data={255};assert(!encoder.updateStatus());assert(callbacks==0&&Wire.requests==0);
 Wire.error=0;Wire.data={};assert(!encoder.updateStatus());assert(callbacks==0);
 Wire.data={0x08};assert(encoder.updateStatus());assert(callbacks>0);
 Wire.data={0x01,0x23};assert(encoder.readCounterInt()==0x123&&Wire.requested==2);
 Wire.data={0xFF};assert(encoder.readCounterInt()==0x123);
 Wire.error=4;assert(encoder.readCounterInt()==0x123);
 puts("PASS: failed status reads cause no input callbacks; valid presses still work; exact two-byte counters; NACK/short reads preserve previous data");
}
''')
 flags=['-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1'] if sys.platform=='darwin' else []
 subprocess.run(['c++','-std=c++17','-DARDUINO=100','-fsanitize=address,undefined',*flags,'-I',str(p),'-I',str(lib),str(p/'test.cpp'),str(lib/'i2cEncoderLibV2.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
