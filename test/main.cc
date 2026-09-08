#include <fmt/core.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "pirate/zip_archive.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::println("usage: {} <zip|tar|tar.gz>", argv[0]);
    return 1;
  }

  const char* zip_file = argv[1];
  try {
    pirate::zip_archive archive(zip_file);
    if (!archive) {
      fmt::println("open failed: {}", zip_file);
      return 1;
    }

    fmt::println("zip_file:{}", zip_file);

    std::vector<pirate::archive_entry> selected;
    for (const auto& entry : archive) {
      fmt::println("file:{} size:{} idx:{} dir:{}", entry.path().string(),
                   entry.file_size(), entry.index(), entry.is_directory());
      if (entry.is_regular_file()) {
        selected.push_back(entry);
      }
    }

    fmt::println("selected:{}", selected.size());
    for (const auto& entry : selected) {
      fmt::println("  pick idx:{} {}", entry.index(), entry.path().string());
    }

    if (!selected.empty()) {
      const int ret = archive.extract<pirate::sequential_extract_factory>(
          selected, pirate::path{"out"});
      fmt::println("extract:{}", ret);
    }
  } catch (const std::exception& e) {
    fmt::println("error{}", e.what());
    return 1;
  }

  return 0;
}
