#ifndef PIRATE_EXTRACT_OPTIONS_H_
#define PIRATE_EXTRACT_OPTIONS_H_

namespace pirate {

// Hint for parallel_extract_factory. sequential_extract_factory ignores it.
struct extract_options {
  unsigned concurrency = 1;
  bool create_directories = true;
};

}  // namespace pirate

#endif
