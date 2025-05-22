#include "base/file.h"

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace pirate {
namespace base {

std::string File::str_mode(File::Mode const mod) {
  if (mod == Mode::ro) {
    return "rb";
  }

  if (mod == Mode::rw_update) {
    return "rb+";
  }

  if (mod == Mode::rw_create) {
    return "wb+";
  }

  if (mod == Mode::wo) {
    return "wb";
  }

  if (mod == Mode::append) {
    return "ab+";
  }

  return "";
}

File::File()
    : stream_{nullptr},
      mode_(Mode::ro),
      file_name_{},
      written_bytes_{0},
      read_bytes_{0} {}

File::File(std::string const &f_name, Mode mod)
    : File(f_name, mod, Positon::begin, 0) {}

File::File(std::string const &f_name, Mode mod, Positon whence, uint64_t offset)
    : stream_{nullptr},
      mode_(mod),
      file_name_(f_name),
      written_bytes_{0},
      read_bytes_{0} {
  stream_ = ::fopen(file_name_.data(), str_mode(mod).data());
  if (stream_ and mod != Mode::append) {
    seek(whence, offset);
  }
}

// File::File(File &&other) noexcept {
//   stream_ = other.stream_;
//   other.stream_ = nullptr;
//   mode_ = other.mode_;
//   file_name_ = std::move(other.file_name_);
//   written_bytes_.store(other.written_bytes_);
//   read_bytes_.store(other.read_bytes_);
// }

// File &File::operator=(File &&other) noexcept {
//   if (this != std::addressof(other)) {
//     stream_ = other.stream_;
//     other.stream_ = nullptr;
//     mode_ = other.mode_;
//     file_name_ = std::move(other.file_name_);
//     written_bytes_.store(other.written_bytes_);
//     read_bytes_.store(other.read_bytes_);
//   }

//   return *this;
// }

File::~File() { close(); }

File::operator FILE *() { return stream_; }

File::operator bool() const { return stream_ != nullptr; }

File &File::open(std::string const &f_name, Mode mod) {
  close();

  file_name_ = f_name;
  mode_ = mod;
  written_bytes_ = 0;
  read_bytes_ = 0;
  stream_ = ::fopen(file_name_.data(), str_mode(mode_).data());

  return *this;
}

std::string File::base_name() const {
  auto const pos = file_name_.find_last_of('/');
  if (pos == std::string::npos) {
    throw std::runtime_error("cannot find base name");
    return "";
  }

  return file_name_.substr(pos + 1);
}

std::string File::path_name() const {
  auto const pos = file_name_.find_last_of('/');
  if (pos == std::string::npos) {
    throw std::runtime_error("cannot find path name");
    return "";
  }

  return file_name_.substr(0, pos);
}

std::string File::full_name() const { return file_name_; }

File &File::seek(Positon whence, uint64_t offset) {
  int fseek_whence = 0;
  switch (whence) {
    case Positon::current:
      fseek_whence = SEEK_CUR;
      break;
    case Positon::end:
      fseek_whence = SEEK_END;
      break;
    default:
      fseek_whence = SEEK_SET;
  }

  ::fseek(stream_, offset, fseek_whence);
  return *this;
}

File &File::write(const char *buf, const size_t len /*50*/) {
  size_t written = 0;

  while (written != len) {
    size_t remain = len - written; /*50*/
    auto n /*20*/ = (long int)fwrite(buf + written, 1, remain, stream_);
    if (n < 0) {
      // int err = ferror(stream_);// FIXME:是都抛出
      break;
    } else {
      buf += n;
      written += n;
    }
  }

  written_bytes_.fetch_add(written);
  return *this;
}

File &File::flush() {
  ::fflush(stream_);
  return *this;
}

size_t File::read(char *buf, size_t expect /*50*/) {
  size_t read_len = 0;

  while (expect != read_len) {
    auto n /*20*/ = ::fread(buf, 1, std::max(read_len, expect), stream_);
    if (n > 0) {
      buf += n;
      read_len += n; /*20*/
    } else {
      break;
    }
  }

  read_bytes_.fetch_add(read_len);
  return read_len;
}

void File::close() {
  if (stream_) {
    ::fclose(stream_);
    stream_ = nullptr;
  }
}

void File::remove() {
  close();
  std::filesystem::remove(file_name_);
  file_name_.clear();
  written_bytes_ = 0;
  read_bytes_ = 0;
}

size_t File::size() const { return std::filesystem::file_size(file_name_); }

size_t File::written_bytes() const { return written_bytes_; }

size_t File::read_bytes() const { return read_bytes_; }

File::Mode File::mode() const { return mode_; }

bool File::exist() const { return std::filesystem::exists(file_name_); }

}  // namespace base
}  // namespace pirate
