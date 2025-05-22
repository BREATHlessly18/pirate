#ifndef PIRATE_UNZ_PIRATE_H_
#define PIRATE_UNZ_PIRATE_H_

#include <minizip/unzip.h>
#include <stdint.h>

#include <string>

#include "pirate/unz_pirate_iterator.h"

namespace pirate {

class unz_pirate {
 public:
  unz_pirate(const std::string& zip_path);
  ~unz_pirate();

  int init();
  void deinit();

  operator bool() const { return handle_ != nullptr; }

  unzFile native_handle() { return handle_; }

  bool no_more() const { return no_more_; }
  void set_no_more(bool n) { no_more_ = n; }

  unz_pirate_iterator compress_file() { return unz_pirate_iterator(this); }

 private:
  bool no_more_{false};
  unzFile handle_{nullptr};
  std::string zip_path_{};
};

}  // namespace pirate

#endif
