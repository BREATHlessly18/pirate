#ifndef PIRATE_TEST_MAKE_ARCHIVE_H_
#define PIRATE_TEST_MAKE_ARCHIVE_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <zlib.h>

namespace pirate_test {

inline void append_u16(std::string& s, std::uint16_t v) {
  s.push_back(static_cast<char>(v & 0xff));
  s.push_back(static_cast<char>((v >> 8) & 0xff));
}

inline void append_u32(std::string& s, std::uint32_t v) {
  s.push_back(static_cast<char>(v & 0xff));
  s.push_back(static_cast<char>((v >> 8) & 0xff));
  s.push_back(static_cast<char>((v >> 16) & 0xff));
  s.push_back(static_cast<char>((v >> 24) & 0xff));
}

inline std::uint32_t crc32_of(const std::string& data) {
  return static_cast<std::uint32_t>(
      ::crc32(0, reinterpret_cast<const Bytef*>(data.data()),
              static_cast<uInt>(data.size())));
}

struct zip_member {
  std::string name;
  std::string data;
  bool is_dir{false};
};

inline void write_bytes(const std::filesystem::path& path, const std::string& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

inline std::string make_store_zip(const std::vector<zip_member>& members) {
  std::string locals;
  std::string centrals;
  for (const auto& m : members) {
    const std::string payload = m.is_dir ? std::string{} : m.data;
    const auto crc = crc32_of(payload);
    const auto sz = static_cast<std::uint32_t>(payload.size());
    const auto name_len = static_cast<std::uint16_t>(m.name.size());
    const auto local_off = static_cast<std::uint32_t>(locals.size());

    locals += "PK\x03\x04";
    append_u16(locals, 20);
    append_u16(locals, 0);
    append_u16(locals, 0);
    append_u16(locals, 0);
    append_u16(locals, 0);
    append_u32(locals, crc);
    append_u32(locals, sz);
    append_u32(locals, sz);
    append_u16(locals, name_len);
    append_u16(locals, 0);
    locals += m.name;
    locals += payload;

    centrals += "PK\x01\x02";
    append_u16(centrals, 20);
    append_u16(centrals, 20);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u32(centrals, crc);
    append_u32(centrals, sz);
    append_u32(centrals, sz);
    append_u16(centrals, name_len);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u16(centrals, 0);
    append_u32(centrals, m.is_dir ? 0x10u : 0u);
    append_u32(centrals, local_off);
    centrals += m.name;
  }

  const auto central_off = static_cast<std::uint32_t>(locals.size());
  const auto central_sz = static_cast<std::uint32_t>(centrals.size());
  const auto n = static_cast<std::uint16_t>(members.size());
  std::string eocd = "PK\x05\x06";
  append_u16(eocd, 0);
  append_u16(eocd, 0);
  append_u16(eocd, n);
  append_u16(eocd, n);
  append_u32(eocd, central_sz);
  append_u32(eocd, central_off);
  append_u16(eocd, 0);
  return locals + centrals + eocd;
}

inline void put_octal(char* dst, std::size_t width, unsigned long long value) {
  std::snprintf(dst, width, "%0*llo", static_cast<int>(width - 1), value);
  dst[width - 1] = '\0';
}

inline std::string make_ustar(const std::vector<zip_member>& members) {
  std::string out;
  for (const auto& m : members) {
    char hdr[512]{};
    std::strncpy(hdr, m.name.c_str(), 99);
    put_octal(hdr + 100, 8, 0644);
    put_octal(hdr + 108, 8, 0);
    put_octal(hdr + 116, 8, 0);
    put_octal(hdr + 124, 12, m.is_dir ? 0 : m.data.size());
    put_octal(hdr + 136, 12, 0);
    std::memset(hdr + 148, ' ', 8);
    hdr[156] = m.is_dir ? '5' : '0';
    std::memcpy(hdr + 257, "ustar", 5);
    hdr[262] = '0';
    hdr[263] = '0';
    unsigned sum = 0;
    for (unsigned char c : hdr) {
      sum += c;
    }
    std::snprintf(hdr + 148, 7, "%06o", sum);
    hdr[154] = '\0';
    hdr[155] = ' ';
    out.append(hdr, 512);
    if (!m.is_dir) {
      out += m.data;
      const auto pad = (512 - (m.data.size() % 512)) % 512;
      out.append(pad, '\0');
    }
  }
  out.append(1024, '\0');
  return out;
}

inline std::string gzip_wrap(const std::string& raw) {
  z_stream zs{};
  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    return {};
  }
  zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(raw.data()));
  zs.avail_in = static_cast<uInt>(raw.size());
  std::string out(raw.size() + 64, '\0');
  int ret = Z_OK;
  std::size_t used = 0;
  while (ret == Z_OK) {
    if (used + 64 > out.size()) {
      out.resize(out.size() * 2);
    }
    zs.next_out = reinterpret_cast<Bytef*>(&out[used]);
    zs.avail_out = static_cast<uInt>(out.size() - used);
    ret = deflate(&zs, Z_FINISH);
    used = zs.total_out;
  }
  deflateEnd(&zs);
  out.resize(used);
  return out;
}

}  // namespace pirate_test

#endif
