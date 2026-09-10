#ifndef PIRATE_ARCHIVE_H_
#define PIRATE_ARCHIVE_H_

#include <cstdint>
#include <vector>

#include "pirate/archive_entry.h"
#include "pirate/archive_iterator.h"
#include "pirate/extract_factory.h"
#include "pirate/extract_options.h"
#include "pirate/path.h"

struct archive;

namespace pirate {

// One reader for zip, tar, and tar.gz (libarchive probes the file).
class archive {
 public:
  explicit archive(path source);
  archive(const archive&) = delete;
  archive& operator=(const archive&) = delete;
  archive(archive&& other) noexcept;
  archive& operator=(archive&& other) noexcept;
  ~archive();

  explicit operator bool() const noexcept { return opened_; }

  const path& location() const noexcept { return source_; }

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

  path source_{};
  ::archive* reader_{nullptr};
  std::uint64_t next_index_{0};
  bool opened_{false};
};

inline archive_iterator begin(archive& ar) { return ar.begin(); }
inline archive_sentinel end(archive& ar) { return ar.end(); }

template <typename Factory = sequential_extract_factory>
int extract(archive& ar, const std::vector<archive_entry>& entries,
            const path& destination_root, extract_options opts = {}) {
  return ar.extract<Factory>(entries, destination_root, opts);
}

}  // namespace pirate

#endif
