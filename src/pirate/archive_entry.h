#ifndef PIRATE_ARCHIVE_ENTRY_H_
#define PIRATE_ARCHIVE_ENTRY_H_

#include <cstdint>

#include "pirate/path.h"

namespace pirate {

class archive_iterator;
class archive;

// Value snapshot of one archive member (like std::filesystem::directory_entry).
// Identified by sequential index in the archive stream (libarchive).
class archive_entry {
 public:
  archive_entry() = default;

  const pirate::path& path() const noexcept { return path_; }
  std::uint64_t file_size() const noexcept { return uncompressed_size_; }
  std::uint64_t compressed_size() const noexcept { return compressed_size_; }
  std::uint64_t index() const noexcept { return index_; }
  bool is_directory() const noexcept { return is_directory_; }
  bool is_regular_file() const noexcept { return !is_directory_; }
  bool has_index() const noexcept { return has_index_; }

#ifdef PIRATE_UNIT_TEST
  static archive_entry with_index(std::uint64_t i, pirate::path p = {}) {
    archive_entry e;
    e.index_ = i;
    e.has_index_ = true;
    e.path_ = std::move(p);
    return e;
  }
#endif

 private:
  friend class archive_iterator;
  friend class archive;

  pirate::path path_{};
  std::uint64_t uncompressed_size_{0};
  std::uint64_t compressed_size_{0};
  std::uint64_t index_{0};
  bool is_directory_{false};
  bool has_index_{false};
};

}  // namespace pirate

#endif
