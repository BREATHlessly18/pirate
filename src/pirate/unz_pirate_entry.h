#ifndef PIRATE_UNZ_PIRATE_ENTRY_H_
#define PIRATE_UNZ_PIRATE_ENTRY_H_

#include <minizip/unzip.h>

#include <string>

namespace pirate {

class unz_pirate_iterator;

class unz_pirate_entry {
 public:
  unz_pirate_entry(unz_pirate_iterator* it);
  unz_pirate_entry(const unz_pirate_entry& rhs);

  bool operator!=(const unz_pirate_entry& rhs) const noexcept {
    return iter_ != rhs.iter_;
  }

  unz64_file_pos get_current_file_pos() const noexcept;
  int get_current_file_info(unz_file_info64& file_info,
                            std::string& uncompress_file_path) const noexcept;

 private:
  unz_pirate_iterator* iter_;
};

}  // namespace pirate

#endif
