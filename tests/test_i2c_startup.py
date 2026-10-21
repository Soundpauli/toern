"""Check startup transport guards against the real linker-wrapper source."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="toern-i2c-startup-") as tmp:
    tmp = Path(tmp)
    (tmp / "Arduino.h").write_text("#pragma once\n#include <stdint.h>\n")
    (tmp / "Wire.h").write_text(
        "#pragma once\nstruct TwoWire {};\nextern TwoWire Wire;\n"
    )
    harness = r'''
#include <cassert>
#include <Wire.h>
#include <stdint.h>
TwoWire Wire, other;
int begins=0, timing=0, writes=0, reads=0;
extern "C" void __real__ZN7TwoWire5beginEv(TwoWire*) { begins++; }
void toernApplyI2cTimingAfterBegin() { assert(begins>0); timing++; }
extern "C" uint8_t __real__ZN7TwoWire15endTransmissionEh(TwoWire*, uint8_t stop) {
  assert(stop==1); writes++; return 0;
}
extern "C" uint8_t __real__ZN7TwoWire11requestFromEhhh(TwoWire*,uint8_t addr,uint8_t len,uint8_t stop) {
  assert(addr==0x30 && len==2 && stop==1); reads++; return len;
}
extern "C" void __wrap__ZN7TwoWire5beginEv(TwoWire*);
extern "C" uint8_t __wrap__ZN7TwoWire15endTransmissionEh(TwoWire*,uint8_t);
extern "C" uint8_t __wrap__ZN7TwoWire11requestFromEhhh(TwoWire*,uint8_t,uint8_t,uint8_t);
int main() {
  // Early encoder accesses must not reach hardware.
  assert(__wrap__ZN7TwoWire15endTransmissionEh(&Wire,1)==4);
  assert(__wrap__ZN7TwoWire11requestFromEhhh(&Wire,0x30,2,1)==0);
  assert(writes==0 && reads==0);
  // Secondary buses remain independent of the primary bus guard.
  assert(__wrap__ZN7TwoWire15endTransmissionEh(&other,1)==0);
  assert(__wrap__ZN7TwoWire11requestFromEhhh(&other,0x30,2,1)==2);
  __wrap__ZN7TwoWire5beginEv(&other);
  assert(timing==0);
  assert(__wrap__ZN7TwoWire15endTransmissionEh(&Wire,1)==4);
  // Codec initialization makes primary Wire ready and applies cable timing.
  __wrap__ZN7TwoWire5beginEv(&Wire);
  assert(timing==1);
  assert(__wrap__ZN7TwoWire15endTransmissionEh(&Wire,1)==0);
  assert(__wrap__ZN7TwoWire11requestFromEhhh(&Wire,0x30,2,1)==2);
  assert(writes==2 && reads==2);
  // A later seesaw begin must restore cable timing again.
  __wrap__ZN7TwoWire5beginEv(&Wire);
  assert(timing==2);
}
'''
    cpp = tmp / "startup.cpp"
    exe = tmp / "startup"
    cpp.write_text(harness)
    cmd = ["c++", "-std=c++17", "-I", str(tmp)]
    sdk = Path("/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1")
    if sdk.exists():
        cmd += ["-isystem", str(sdk)]
    subprocess.run(cmd + [str(cpp), str(root / "toern_i2c.cpp"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("PASS: early I2C accesses skipped; codec enables transfers; repeated begin restores timing; secondary buses unchanged.")
