#include "pirate/unz_pirate.h"

#include <minizip/unzip.h>
#include <stdio.h>

#include <filesystem>

namespace pirate {

unz_pirate::unz_pirate(const std::string& zip_path) : zip_path_(zip_path) {
  init();
}

unz_pirate::~unz_pirate() { deinit(); }

int unz_pirate::init() {
  if (nullptr == handle_) {
    handle_ = unzOpen64(zip_path_.data());
    if (handle_) {
      if (UNZ_OK != unzGoToFirstFile(handle_)) {
        printf("unzGoToFirstFile failed\n");
      }
    }
  }

  return handle_ ? 0 : -1;
}

void unz_pirate::deinit() {
  if (handle_) {
    unzClose(handle_);
  }
}

}  // namespace pirate
