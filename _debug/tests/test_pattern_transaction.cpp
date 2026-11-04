#include <cassert>
#include <map>
#include <string>
#include <stdexcept>
#include <iostream>
#include "../src/pattern_transaction.h"
struct FS {
  std::map<std::string, std::string> files;
  int operation = 0, failAt = -1, powerAt = -1;
  bool exists(const char *p) { return files.count(p); }
  bool step() { if (++operation == powerAt) throw std::runtime_error("power loss"); return operation != failAt; }
  bool remove(const char *p) { return step() && files.erase(p); }
  bool rename(const char *a, const char *b) {
    if (!step() || !exists(a) || exists(b)) return false;
    files[b] = files[a]; files.erase(a); return true;
  }
  bool valid(const char *p, bool full) {
    return exists(p) && files[p] != "partial" && (!full || files[p] != "legacy");
  }
};
bool commit(FS &fs) {
 return commitPatternTransaction(fs,"42.txt","42.txt.tmp","42.txt.bak",
   [&](const char *p, bool full) { return fs.valid(p,full); });
}
bool recover(FS &fs) {
 return recoverPatternTransaction(fs,"42.txt","42.txt.tmp","42.txt.bak",
   [&](const char *p, bool full) { return fs.valid(p,full); });
}
int main() {
 for (int fault = 0; fault < 2; fault++) for (int at = 1; at <= 5; at++) {
   FS fs; fs.files = {{"42.txt","old"},{"42.txt.tmp","new"},{"42.txt.bak","older"},
                       {"autosaved.txt","session"},{"43.txt.tmp","unrelated"}};
   if (fault) fs.powerAt = at; else fs.failAt = at;
   try { commit(fs); } catch (const std::runtime_error &) {}
   fs.failAt = fs.powerAt = -1;
   assert(recover(fs));
   assert(fs.files["42.txt"] == "old" || fs.files["42.txt"] == "new");
   assert(fs.files["autosaved.txt"] == "session" && fs.files["43.txt.tmp"] == "unrelated");
 }
 FS fs; fs.files = {{"42.txt","old"},{"42.txt.tmp","partial"}};
 assert(!commit(fs)); assert(fs.files["42.txt"] == "old");
 fs.files = {{"42.txt.tmp","partial"},{"42.txt.bak","legacy"}};
 assert(recover(fs)); assert(fs.files["42.txt"] == "legacy");
 fs.files = {{"42.txt.tmp","new"}}; assert(recover(fs)); assert(fs.files["42.txt"] == "new");
 fs.files = {{"42.txt","old"},{"42.txt.tmp","new"}};
 assert(recover(fs)); assert(fs.files["42.txt"] == "old");
 fs.files = {{"42.txt","old"},{"42.txt.tmp","empty-with-settings"}};
 assert(commit(fs)); assert(fs.files["42.txt"] == "empty-with-settings");
 assert(deletePatternTransaction(fs,"42.txt","42.txt.tmp","42.txt.bak"));
 assert(!recover(fs)); assert(!fs.exists("42.txt"));
 for (int at = 1; at <= 3; at++) {
   FS deletion;
   deletion.files = {{"42.txt","old"},{"42.txt.tmp","new"},{"42.txt.bak","older"},
                     {"autosaved.txt","session"},{"43.txt","other"}};
   deletion.failAt = at;
   assert(!deletePatternTransaction(deletion,"42.txt","42.txt.tmp","42.txt.bak"));
   assert(deletion.files["42.txt"] == "old");
   assert(deletion.files["autosaved.txt"] == "session" && deletion.files["43.txt"] == "other");
 }
 FS vacant;
 assert(deletePatternTransaction(vacant,"42.txt","42.txt.tmp","42.txt.bak"));
 fs.files = {{"pattern.tmp","new"},{"43.txt.tmp","new"}};
 assert(!recover(fs)); assert(!fs.exists("42.txt"));
 std::cout << "Pattern transaction failure/recovery cases passed\n";
}
