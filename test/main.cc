// #include <zlib.h>

#include <fmt/core.h>
#include <minizip/unzip.h>
#include <unistd.h>

#include <cstdlib>

#include "pirate/unz_handle.h"
#include "pirate/unz_pirate.h"

void traverse_file(const char *zip_file) {
  pirate::unz_pirate uz(zip_file);
  fmt::print("zip_file:{}\n", zip_file);
  if (not uz) {
    fmt::print("unz_pirate init failed\n");
    exit(0);
  }

  for (auto entry : uz.compress_file()) {
    sleep(1);
    unz_file_info64 file_info{};
    std::string uncompress_file_path(128, '\0');
    entry.get_current_file_info(file_info, uncompress_file_path);
    fmt::print("file:{}\n", std::string(uncompress_file_path));
    // pirate::unz_pirate_handle_extract(entry);
  }
}

int main() {
  traverse_file("./xxx.zip");
  return 0;
}
