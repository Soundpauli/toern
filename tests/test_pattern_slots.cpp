#include "../src/toern_pattern_slots.h"
#include <cassert>
#include <cstring>
#include <initializer_list>
using namespace toern_pattern_slots;
int main() {
  char name[24];
  assert(path(name, sizeof(name), 1, 0, false) && !strcmp(name, "1.txt"));
  assert(path(name, sizeof(name), 1, 0, true) && !strcmp(name, "1-A.txt"));
  assert(path(name, sizeof(name), 999, 4, false) && !strcmp(name, "999-E.txt"));
  for (unsigned f : {1u, 99u, 999u}) for (uint8_t s = 0; s < count; ++s) {
    assert(path(name, sizeof(name), f, s, true));
    assert(strchr(name, '-') && strlen(name) < sizeof(name));
  }
  assert(!path(name, sizeof(name), 0, 0, false));
  assert(!path(name, sizeof(name), 1000, 0, true));
  assert(!path(name, sizeof(name), 1, 5, true));
  assert(!path(name, 3, 999, 4, true));
  assert(atBoundary(true, 1, 16) && !atBoundary(true, 17, 16));
  assert(atBoundary(false, 17, 16) && !atBoundary(false, 16, 16));
  assert(atBoundary(false, 33, 32) && !atBoundary(false, 0, 16));
  assert(!atBoundary(false, 1, 0));
  assert(select(0, -1) == 0 && select(4, 1) == 4);
  assert(select(0, 25) == 4 && select(4, -25) == 0);
}
