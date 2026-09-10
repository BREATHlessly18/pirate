#ifndef PIRATE_ARCHIVE_ITERATOR_H_
#define PIRATE_ARCHIVE_ITERATOR_H_

#include <cstddef>
#include <iterator>

#include "pirate/archive_entry.h"

namespace pirate {

class archive;

struct archive_sentinel {};

// Input iterator over archive members (like std::filesystem::directory_iterator).
// Copies share the archive cursor: do not increment two copies independently.
class archive_iterator {
 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = archive_entry;
  using difference_type = std::ptrdiff_t;
  using pointer = const archive_entry*;
  using reference = const archive_entry&;

  archive_iterator() = default;
  explicit archive_iterator(archive* owner);

  reference operator*() const noexcept { return current_; }
  pointer operator->() const noexcept { return &current_; }

  archive_iterator& operator++();
  void operator++(int) { ++*this; }

  bool at_end() const noexcept { return at_end_; }

 private:
  void load_current();

  archive* archive_{nullptr};
  archive_entry current_{};
  bool at_end_{true};
};

inline bool operator==(const archive_iterator& it,
                       archive_sentinel) noexcept {
  return it.at_end();
}

inline bool operator!=(const archive_iterator& it,
                       archive_sentinel s) noexcept {
  return !(it == s);
}

inline bool operator==(archive_sentinel s, const archive_iterator& it) noexcept {
  return it == s;
}

inline bool operator!=(archive_sentinel s, const archive_iterator& it) noexcept {
  return it != s;
}

}  // namespace pirate

#endif
