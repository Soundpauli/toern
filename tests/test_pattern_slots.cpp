#include "../src/toern_pattern_slots.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
using namespace toern_pattern_slots;
int main() {
  char name[24];unsigned file;uint8_t slot;
  assert(parse("42.txt",file,slot)&&file==42&&slot==0);
  assert(parse("999-E.txt",file,slot)&&file==999&&slot==4);
  for(const char *bad:{"42-A.txt","42.txt.tmp","42-B.txt.bak","042.txt","0.txt","1000.txt","samples/42.txt"}) assert(!parse(bad,file,slot));
  assert(countSlots(31)==5&&countSlots(12)==2&&countSlots(0)==0);
  assert(path(name, sizeof(name), 1, 0) && !strcmp(name, "1.txt"));
  assert(path(name, sizeof(name), 999, 4) && !strcmp(name, "999-E.txt"));
  for (unsigned f : {1u, 99u, 999u}) for (uint8_t s = 0; s < count; ++s) {
    assert(path(name, sizeof(name), f, s));
    assert((strchr(name, '-') != nullptr) == (s != 0));
    assert(strlen(name) < sizeof(name));
  }
  assert(!path(name, sizeof(name), 0, 0));
  assert(!path(name, sizeof(name), 1000, 0));
  assert(!path(name, sizeof(name), 1, 5));
  assert(!path(name, 3, 999, 4));
  assert(atBoundary(true, 1, 16) && !atBoundary(true, 17, 16));
  assert(atBoundary(false, 17, 16) && !atBoundary(false, 16, 16));
  assert(atBoundary(false, 33, 32) && !atBoundary(false, 0, 16));
  assert(!atBoundary(false, 1, 0));
  assert(select(0, -1) == 0 && select(4, 1) == 4);
  assert(select(0, 25) == 4 && select(4, -25) == 0);
}
