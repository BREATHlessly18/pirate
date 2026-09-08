#ifndef PIRATE_ZIP_ARCHIVE_H_
#define PIRATE_ZIP_ARCHIVE_H_

#include <cstdint>
#include <vector>

#include "pirate/archive_entry.h"
#include "pirate/archive_iterator.h"
#include "pirate/extract_factory.h"
#include "pirate/extract_options.h"
#include "pirate/path.h"

struct archive;

namespace pirate {

class zip_archive {
 public:
  explicit zip_archive(path zip_path);
  zip_archive(const zip_archive&) = delete;
  zip_archive& operator=(const zip_archive&) = delete;
  zip_archive(zip_archive&& other) noexcept;
  zip_archive& operator=(zip_archive&& other) noexcept;
  ~zip_archive();

  explicit operator bool() const noexcept { return opened_; }

  const path& location() const noexcept { return zip_path_; }

  archive_iterator begin();
  archive_sentinel end() const noexcept { return {}; }

  template <typename Factory = sequential_extract_factory>
  int extract(const archive_entry& entry, const path& destination_root,
              extract_options opts = {}) {
    return extract<Factory>(std::vector<archive_entry>{entry}, destination_root,
                            opts);
  }

  template <typename Factory = sequential_extract_factory>
  int extract(const std::vector<archive_entry>& entries,
              const path& destination_root, extract_options opts = {}) {
    return Factory::extract(*this, entries, destination_root, opts);
  }

 private:
  friend class archive_iterator;
  friend struct sequential_extract_factory;
  friend struct parallel_extract_factory;

  bool open_for_read();
  void close_reader();
  int extract_matching(const std::vector<archive_entry>& entries,
                       const path& destination_root,
                       const extract_options& opts);

  path zip_path_{};
  struct archive* reader_{nullptr};
  std::uint64_t next_index_{0};
  bool opened_{false};
};

inline archive_iterator begin(zip_archive& archive) { return archive.begin(); }
inline archive_sentinel end(zip_archive& archive) { return archive.end(); }

using tar_archive = zip_archive;

template <typename Factory = sequential_extract_factory>
int extract(zip_archive& archive, const std::vector<archive_entry>& entries,
            const path& destination_root, extract_options opts = {}) {
  return archive.extract<Factory>(entries, destination_root, opts);
}

}  // namespace pirate

#endif
