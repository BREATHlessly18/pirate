#include "pirate/zip_archive.h"

#include <archive.h>
#include <archive_entry.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pirate {
namespace {

namespace fs = std::filesystem;

bool is_zip_slip(const std::string& member) {
  fs::path p = fs::path(member).lexically_normal();
  for (const auto& part : p) {
    if (part == "..") {
      return true;
    }
  }
  return false;
}

fs::path destination_for(const path& root, const std::string& member_path,
                         bool is_directory) {
  std::string rel = member_path;
  while (!rel.empty() && (rel.back() == '/' || rel.back() == '\\')) {
    rel.pop_back();
  }
  fs::path dest = fs::path(root.native());
  for (const auto& part : fs::path(rel)) {
    dest /= part;
  }
  (void)is_directory;
  return dest;
}

int write_current_data(struct archive* reader, const fs::path& dest) {
  std::ofstream out(dest, std::ios::binary | std::ios::trunc);
  if (!out) {
    return -1;
  }

  char buf[8192];
  la_ssize_t n = 0;
  while ((n = archive_read_data(reader, buf, sizeof(buf))) > 0) {
    out.write(buf, static_cast<std::streamsize>(n));
    if (!out) {
      return -1;
    }
  }
  return n < 0 ? -1 : 0;
}

}  // namespace

zip_archive::zip_archive(path zip_path) : zip_path_(std::move(zip_path)) {
  for (unsigned char c : zip_path_.native()) {
    if (c > 127) {
      throw std::runtime_error("non-ASCII zip path is not supported");
    }
  }
  opened_ = open_for_read();
}

zip_archive::zip_archive(zip_archive&& other) noexcept
    : zip_path_(std::move(other.zip_path_)),
      reader_(other.reader_),
      next_index_(other.next_index_),
      opened_(other.opened_) {
  other.reader_ = nullptr;
  other.opened_ = false;
  other.next_index_ = 0;
}

zip_archive& zip_archive::operator=(zip_archive&& other) noexcept {
  if (this != &other) {
    close_reader();
    zip_path_ = std::move(other.zip_path_);
    reader_ = other.reader_;
    next_index_ = other.next_index_;
    opened_ = other.opened_;
    other.reader_ = nullptr;
    other.opened_ = false;
    other.next_index_ = 0;
  }
  return *this;
}

zip_archive::~zip_archive() { close_reader(); }

void zip_archive::close_reader() {
  if (reader_) {
    archive_read_free(reader_);
    reader_ = nullptr;
  }
}

bool zip_archive::open_for_read() {
  close_reader();
  next_index_ = 0;
  reader_ = archive_read_new();
  if (!reader_) {
    opened_ = false;
    return false;
  }

  archive_read_support_filter_all(reader_);
  archive_read_support_format_zip(reader_);
  archive_read_support_format_tar(reader_);

  const int ret =
      archive_read_open_filename(reader_, zip_path_.native().data(), 10240);
  if (ret != ARCHIVE_OK) {
    close_reader();
    opened_ = false;
    return false;
  }
  opened_ = true;
  return true;
}

archive_iterator zip_archive::begin() {
  if (!open_for_read()) {
    return archive_iterator{};
  }
  return archive_iterator{this};
}

int zip_archive::extract_matching(const std::vector<archive_entry>& entries,
                                  const path& destination_root,
                                  const extract_options& opts) {
  if (!open_for_read()) {
    return -1;
  }

  std::unordered_set<std::uint64_t> wanted;
  wanted.reserve(entries.size());
  for (const auto& entry : entries) {
    if (!entry.has_index() || is_zip_slip(entry.path().native())) {
      return -1;
    }
    wanted.insert(entry.index());
  }

  std::uint64_t index = 0;
  ::archive_entry* header = nullptr;
  int ret = ARCHIVE_OK;
  while ((ret = archive_read_next_header(reader_, &header)) == ARCHIVE_OK ||
         ret == ARCHIVE_WARN) {
    if (wanted.find(index) != wanted.end()) {
      const char* name = archive_entry_pathname(header);
      std::string name_str = name ? name : "";
      const bool is_dir =
          archive_entry_filetype(header) == AE_IFDIR ||
          (!name_str.empty() &&
           (name_str.back() == '/' || name_str.back() == '\\'));
      const fs::path dest =
          destination_for(destination_root, name_str, is_dir);
      if (opts.create_directories) {
        const fs::path dir = is_dir ? dest : dest.parent_path();
        if (!dir.empty()) {
          std::error_code ec;
          fs::create_directories(dir, ec);
          if (ec) {
            return -1;
          }
        }
      }
      if (!is_dir) {
        if (write_current_data(reader_, dest) != 0) {
          return -1;
        }
      }
      wanted.erase(index);
      if (wanted.empty()) {
        return 0;
      }
    } else if (archive_read_data_skip(reader_) < ARCHIVE_OK) {
      return -1;
    }
    ++index;
  }

  return (ret == ARCHIVE_EOF && wanted.empty()) ? 0 : -1;
}

int sequential_extract_factory::extract(zip_archive& archive,
                                        const std::vector<archive_entry>& entries,
                                        const path& destination_root,
                                        extract_options opts) {
  (void)opts.concurrency;
  return archive.extract_matching(entries, destination_root, opts);
}

int parallel_extract_factory::extract(zip_archive& archive,
                                      const std::vector<archive_entry>& entries,
                                      const path& destination_root,
                                      extract_options opts) {
  // One zip_archive / unz handle per worker later; partition by entry.index().
  return sequential_extract_factory::extract(archive, entries, destination_root,
                                             opts);
}

}  // namespace pirate
