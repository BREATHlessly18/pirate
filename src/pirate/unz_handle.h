#ifndef PIRATE_UNZ_PIRATE_HANDLE_H_
#define PIRATE_UNZ_PIRATE_HANDLE_H_

#include "pirate/unz_pirate.h"

namespace pirate {

class unz_pirate_entry;

struct unz_pirate_handle_extract {
  int operator()(unz_pirate_entry& entry) const noexcept;
};

}  // namespace pirate

#endif
