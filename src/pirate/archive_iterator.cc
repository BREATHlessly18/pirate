#include "pirate/archive_iterator.h"

#include <archive.h>
#include <archive_entry.h>

#include <string>

#include "pirate/zip_archive.h"

namespace pirate {
namespace {

bool looks_like_directory(const char* name, unsigned int file_type) {
  if (file_type == AE_IFDIR) {
    return true;
  }
  if (name == nullptr || name[0] == '\0') {
    return false;
  }
  const std::string s(name);
  return s.back() == '/' || s.back() == '\\';
}

}  // namespace

archive_iterator::archive_iterator(zip_archive* archive)
    : archive_(archive), at_end_(archive == nullptr || !*archive) {
  if (!at_end_) {
    load_current();
  }
}

void archive_iterator::load_current() {
  current_ = archive_entry{};
  if (!archive_ || !archive_->reader_) {
    at_end_ = true;
    return;
  }

  ::archive_entry* header = nullptr;
  const int ret = archive_read_next_header(archive_->reader_, &header);
  if (ret == ARCHIVE_EOF) {
    at_end_ = true;
    return;
  }
  if (ret != ARCHIVE_OK && ret != ARCHIVE_WARN) {
    at_end_ = true;
    return;
  }

  const char* name = archive_entry_pathname(header);
  current_.path_ = pirate::path{name ? name : ""};
  current_.uncompressed_size_ =
      static_cast<std::uint64_t>(archive_entry_size(header));
  current_.index_ = archive_->next_index_++;
  current_.has_index_ = true;
  current_.is_directory_ = looks_like_directory(
      name, static_cast<unsigned int>(archive_entry_filetype(header)));
}

archive_iterator& archive_iterator::operator++() {
  if (at_end_ || !archive_ || !archive_->reader_) {
    at_end_ = true;
    return *this;
  }
  archive_read_data_skip(archive_->reader_);
  load_current();
  return *this;
}

}  // namespace pirate
