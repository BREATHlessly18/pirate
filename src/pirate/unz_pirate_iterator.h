#ifndef UNZ_PIRATE_ITERATOR_H_
#define UNZ_PIRATE_ITERATOR_H_

#include <minizip/unzip.h>

#include "pirate/unz_pirate_entry.h"

namespace pirate {

class unz_pirate;
// class unz_pirate_entry;

class unz_pirate_iterator_proxy {
 public:
  const unz_pirate_entry& operator*() const& noexcept;
  unz_pirate_entry& operator*() && noexcept;

 private:
  friend class unz_pirate_iterator;
  explicit unz_pirate_iterator_proxy(const unz_pirate_entry& entry);

 private:
  unz_pirate_entry entry_;
};

class unz_pirate_iterator {
 public:
  unz_pirate_iterator() : pirate_(nullptr) {}
  unz_pirate_iterator(unz_pirate* pirate) : pirate_(pirate) {}

  unz_pirate_iterator& operator++();
  unz_pirate_iterator_proxy operator++(int);

  unz_pirate_entry operator*();
  unz_pirate_iterator* operator->();

  unz64_file_pos get_current_file_pos() noexcept;
  int get_current_file_info(unz_file_info64& file_info,
                            std::string& uncompress_file_path) noexcept;

  // int get_current_file_infocpnz_file_info &file_info,

  friend bool operator!=(const unz_pirate_iterator& lhs,
                         const unz_pirate_iterator& rhs) noexcept;

 private:
  unz_pirate* pirate_;
  unz64_file_pos compressed_file_pos_;
};

inline unz_pirate_iterator begin(unz_pirate_iterator iter) noexcept {
  return iter;
}

inline unz_pirate_iterator end(unz_pirate_iterator iter) noexcept {
  return unz_pirate_iterator{};
}

bool operator!=(const unz_pirate_iterator& lhs,
                const unz_pirate_iterator& rhs) noexcept;

// Ehars const uncompress_file_path);
// friend bool operator!=(const unz_pirate_iterator& lhs, 4B pirate* pirate_;
//                        const unz_pirate_iterator& rhs) noexcept;
// 49 unz64_file_pos compressed_file_pos_;
// inline pirate_1terator begin(unz_pirate_iterator
//                                  /// Return a past-the-end pirate_1terator
//                                  L_iter) noexcept {
//   return iter;
//   55 anlane unz_pirate_iterator end(pirate_1terator) noexcept {
// 56
// return pirate_iteratorO)il
// 574 bool, operator!=(const unz_pirate_iterator& Lhs,
// 58
// const unz_pirate_iterator& rhs) noexcept;

}  // namespace pirate

#endif
