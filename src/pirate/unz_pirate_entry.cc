#include "pirate/unz_pirate_entry.h"

#include "pirate/unz_pirate_iterator.h"

namespace pirate {
unz_pirate_entry::unz_pirate_entry(unz_pirate_iterator* it) : iter_(it) {}
unz_pirate_entry::unz_pirate_entry(const unz_pirate_entry& rhs)
    : iter_(rhs.iter_) {}

unz64_file_pos unz_pirate_entry::get_current_file_pos() const noexcept {
  return iter_->get_current_file_pos();
}

int unz_pirate_entry::get_current_file_info(
    unz_file_info64& file_info,
    std::string& uncompress_file_path) const noexcept {
  return iter_->get_current_file_info(file_info, uncompress_file_path);
}

}  // namespace pirate
