#ifndef PIRATE_EXTRACT_FACTORY_H_
#define PIRATE_EXTRACT_FACTORY_H_

#include <vector>

#include "pirate/archive_entry.h"
#include "pirate/extract_options.h"
#include "pirate/path.h"

namespace pirate {

class zip_archive;

// Policy factories (synchronous_factory / async_factory).
// Chosen at compile time: archive.extract<parallel_extract_factory>(...)

struct sequential_extract_factory {
  static int extract(zip_archive& archive,
                     const std::vector<archive_entry>& entries,
                     const path& destination_root, extract_options opts);
};

// Same API; will open one zip_archive per worker later. Currently one pass.
struct parallel_extract_factory {
  static int extract(zip_archive& archive,
                     const std::vector<archive_entry>& entries,
                     const path& destination_root, extract_options opts);
};

}  // namespace pirate

#endif
