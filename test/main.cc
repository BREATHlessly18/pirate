#include <fmt/core.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "pirate/archive.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::println("usage: {} <zip|tar|tar.gz>", argv[0]);
    return 1;
  }

  const char* archive_file = argv[1];
  try {
    pirate::archive ar(archive_file);
    if (!ar) {
      fmt::println("open failed: {}", archive_file);
      return 1;
    }

    fmt::println("archive:{}", archive_file);

    std::vector<pirate::archive_entry> selected;
    for (const auto& entry : ar) {
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
      const int ret = ar.extract<pirate::sequential_extract_factory>(
          selected, pirate::path{"out"});
      fmt::println("extract:{}", ret);
    }
  } catch (const std::exception& e) {
    fmt::println("error{}", e.what());
    return 1;
  }

  return 0;
}
