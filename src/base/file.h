#ifndef PIRATE_BASE_FILE_H_
#define PIRATE_BASE_FILE_H_

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>

#include <atomic>
#include <string>

#include "base/copyable.h"

namespace pirate {
namespace base {

class File {
public:
  enum class Mode {
    ro,        // 只读, 文件必须存在, 原内容保留
    rw_update, // 读写, 文件必须存在, 原内容保留
    rw_create, // 读写, 无文件则创建, 目标文件内容制空
    wo,        // 只写, 无文件则创建, 目标文件内容制空
    append, // 只写, 无文件则创建, 目标文件末尾追加，无法seek
    no_rw   // 支持不进行读写操作访问文件
  };

  enum class Positon {
    begin,   // 从头开始
    current, // 当前开始
    end,     // 尾部开始
  };

public:
  File();
  File(std::string const &f_name, Mode mod);
  File(std::string const &f_name, Mode mod, Positon whence, uint64_t offset);

  ~File();
  operator FILE *();
  operator bool() const;

  std::string base_name() const;
  std::string path_name() const;
  std::string full_name() const;

  File &open(std::string const &f_name, Mode mod);
  File &seek(Positon whence, uint64_t offset);
  File &write(const char *buf, size_t len);
  File &flush();

  size_t read(char *buf, size_t expect);
  void close();
  void remove();

  size_t size() const;
  size_t written_bytes() const;
  size_t read_bytes() const;
  bool exist() const;
  Mode mode() const;

private:
  static std::string str_mode(Mode const mod);

private:
  FILE *stream_;
  Mode mode_;
  std::string file_name_;
  std::atomic<off_t> written_bytes_;
  std::atomic<off_t> read_bytes_;
};

} // namespace base
} // namespace pirate

#endif