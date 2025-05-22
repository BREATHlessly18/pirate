#include "pirate/unz_pirate_iterator.h"

#include <minizip/unzip.h>

#include <cstdio>
#include <memory>

#include "pirate/unz_pirate.h"

namespace pirate {
const unz_pirate_entry& unz_pirate_iterator_proxy::operator*() const& noexcept {
  return entry_;
}

unz_pirate_entry& unz_pirate_iterator_proxy::operator*() && noexcept {
  return entry_;
}

unz_pirate_iterator_proxy::unz_pirate_iterator_proxy(
    const unz_pirate_entry& entry)
    : entry_(entry) {}

unz_pirate_iterator& unz_pirate_iterator::operator++() {
  int ret = unzGoToNextFile(pirate_->native_handle());
  if (UNZ_OK == ret) {
  } else if (UNZ_END_OF_LIST_OF_FILE == ret) {
    pirate_->set_no_more(true);
  } else if (UNZ_ERRNO == ret) {
    printf("unzGoToNextFile failed\n");
  }

  return *this;
}

unz_pirate_iterator_proxy unz_pirate_iterator::operator++(int) {
  int ret = unzGoToNextFile(pirate_->native_handle());
  if (UNZ_OK == ret) {
  } else if (UNZ_END_OF_LIST_OF_FILE == ret) {
    pirate_->set_no_more(true);
  } else if (UNZ_ERRNO == ret) {
    printf("unzGoToNextFile failed\n");
  }

  unz_pirate_entry entry(this);
  unz_pirate_iterator_proxy proxy{entry};
  return proxy;
}

unz64_file_pos unz_pirate_iterator::get_current_file_pos() noexcept {
  unzGetFilePos64(pirate_->native_handle(), &compressed_file_pos_);
  return compressed_file_pos_;
}

int unz_pirate_iterator::get_current_file_info(
    unz_file_info64& file_info, std::string& uncompress_file_path) noexcept {
  int ret = unzGetCurrentFileInfo64(
      pirate_->native_handle(), &file_info, uncompress_file_path.data(),
      uncompress_file_path.size(), nullptr, 0, nullptr, 0);
  return ret;
}

unz_pirate_entry unz_pirate_iterator::operator*() {
  // unz_pirate_entry entry(this);
  return this;
}

unz_pirate_iterator* unz_pirate_iterator::operator->() { return this; }

bool operator!=(const unz_pirate_iterator& lhs,
                const unz_pirate_iterator& rhs) noexcept {
  if (&lhs == &rhs) {
    return true;
  }

  if (lhs.pirate_ == rhs.pirate_) {
    return false;
  }

  if (lhs.pirate_ && not lhs.pirate_->no_more()) {
    return true;
  }

  if (rhs.pirate_ && not rhs.pirate_->no_more()) {
    return true;
  }

  return false;
}

}  // namespace pirate
