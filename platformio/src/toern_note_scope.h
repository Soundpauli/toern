#pragma once
// Pointer-only scope: nested timer callbacks restore the interrupted grid.
template<class Grid> struct ToernNoteScope {
  Grid &slot, saved;
  ToernNoteScope(Grid &slot, Grid target):slot(slot),saved(slot){slot=target;}
  ~ToernNoteScope(){slot=saved;}
};
