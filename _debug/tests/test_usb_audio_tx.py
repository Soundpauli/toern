import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = (Path(__file__).resolve().parents[2] / 'platformio')
spec = importlib.util.spec_from_file_location('patch_usb', ROOT/'scripts/patch_usb_audio.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class UsbTx(unittest.TestCase):
    def test_patch_is_idempotent_and_rejects_unknown_sources(self):
        original = (ROOT/'../_debug/tests/fixtures/usb_audio_tx.cpp').read_text()
        patched = module.patch_usb_audio(original)
        self.assertEqual(patched, module.patch_usb_audio(patched))
        with self.assertRaises(RuntimeError):
            module.patch_usb_audio('unrecognised framework')

    def test_actual_callback_preserves_prefix_and_clears_entire_tail(self):
        patched = module.patch_usb_audio((ROOT/'../_debug/tests/fixtures/usb_audio_tx.cpp').read_text())
        callback = patched[patched.index('void AudioOutputUSB::update(void)'):]
        prefix = r'''
#include <stdint.h>
#include <string.h>
#include <assert.h>
#define AUDIO_BLOCK_SAMPLES 128
struct audio_block_t { int16_t data[128]; };
struct AudioStream { static void release(audio_block_t*) {} };
struct AudioOutputUSB { static audio_block_t *left_1st,*right_1st,*left_2nd,*right_2nd; static unsigned offset_1st; static audio_block_t *incoming[2]; static audio_block_t* receiveWritable(int i){return incoming[i];} static audio_block_t* allocate(){return nullptr;} static void release(audio_block_t*){}; static void update(); };
audio_block_t *AudioOutputUSB::left_1st=nullptr,*AudioOutputUSB::right_1st=nullptr,*AudioOutputUSB::left_2nd=nullptr,*AudioOutputUSB::right_2nd=nullptr;
unsigned AudioOutputUSB::offset_1st=0;
audio_block_t *AudioOutputUSB::incoming[2]={nullptr,nullptr};
uint8_t usb_audio_transmit_setting=1;
#define __disable_irq() ((void)0)
#define __enable_irq() ((void)0)
uint32_t toern_usb_tx_overruns=0,toern_usb_tx_discarded_frames=0;
uint32_t toern_usb_tx_underruns=0,toern_usb_tx_silent_frames=0;
uint16_t usb_audio_transmit_buffer[128];
static void copy_from_buffers(uint32_t *dst,int16_t*l,int16_t*r,unsigned n) {
 while(n--) *dst++ = (uint32_t(uint16_t(*r++))<<16)|uint16_t(*l++);
}
'''
        main = r'''
int main() {
 audio_block_t l{},r{};
 for(int i=0;i<128;i++){l.data[i]=123;r.data[i]=456;}
 // Empty queue: all packet frames zero, including last stereo frame.
 memset(usb_audio_transmit_buffer,0x55,sizeof usb_audio_transmit_buffer);
 unsigned bytes=usb_audio_transmit_callback();
 for(unsigned i=0;i<bytes/2;i++) assert(usb_audio_transmit_buffer[i]==0);
 assert(toern_usb_tx_underruns==1 && toern_usb_tx_silent_frames==bytes/4);
 // Partial queue: first 7 stereo frames survive; remainder zeroed.
 AudioOutputUSB::left_1st=&l;AudioOutputUSB::right_1st=&r;AudioOutputUSB::offset_1st=121;
 memset(usb_audio_transmit_buffer,0x55,sizeof usb_audio_transmit_buffer);
 bytes=usb_audio_transmit_callback();
 for(unsigned i=0;i<7;i++){assert(usb_audio_transmit_buffer[i*2]==123);assert(usb_audio_transmit_buffer[i*2+1]==456);}
 for(unsigned i=14;i<bytes/2;i++) assert(usb_audio_transmit_buffer[i]==0);
 assert(usb_audio_transmit_buffer[bytes/2]==0x5555);
 assert(toern_usb_tx_underruns==2);
 assert(toern_usb_tx_silent_frames==(bytes/4)*2-7);
 // Normal full packet: no artificial silence or underrun increments.
 AudioOutputUSB::left_1st=&l;AudioOutputUSB::right_1st=&r;AudioOutputUSB::offset_1st=0;
 bytes=usb_audio_transmit_callback();
 for(unsigned i=0;i<bytes/4;i++){assert(usb_audio_transmit_buffer[2*i]==123);assert(usb_audio_transmit_buffer[2*i+1]==456);}
 assert(toern_usb_tx_underruns==2);
 // Overrun: count only the unconsumed portion of the discarded block.
 AudioOutputUSB::left_1st=&l;AudioOutputUSB::right_1st=&r;
 AudioOutputUSB::left_2nd=&l;AudioOutputUSB::right_2nd=&r;
 AudioOutputUSB::offset_1st=7;
 AudioOutputUSB::incoming[0]=&l;AudioOutputUSB::incoming[1]=&r;
 AudioOutputUSB::update();
 assert(toern_usb_tx_overruns==1 && toern_usb_tx_discarded_frames==121);
 assert(AudioOutputUSB::offset_1st==0);
}
'''
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/'test.cpp';exe=Path(d)/'test'
            src.write_text(prefix+callback+main)
            subprocess.run(['c++','-std=c++17',str(src),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

if __name__ == '__main__': unittest.main()
