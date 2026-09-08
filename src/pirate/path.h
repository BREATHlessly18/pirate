#ifndef PIRATE_PATH_H_
#define PIRATE_PATH_H_

#include <string>
#include <utility>

namespace pirate {

class path {
 public:
  path() = default;
  path(std::string s) : native_(std::move(s)) {}
  path(const char* s) : native_(s ? s : "") {}

  const std::string& native() const noexcept { return native_; }
  std::string string() const { return native_; }
  bool empty() const noexcept { return native_.empty(); }

  path filename() const {
    const auto pos = native_.find_last_of("/\\");
    if (pos == std::string::npos) {
      return path{native_};
    }
    return path{native_.substr(pos + 1)};
  }

  path parent() const {
    const auto pos = native_.find_last_of("/\\");
    if (pos == std::string::npos || pos == 0) {
      return path{};
    }
    return path{native_.substr(0, pos)};
  }

 private:
  std::string native_;
};

inline path operator/(const path& lhs, const path& rhs) {
  if (lhs.empty()) {
    return rhs;
  }
  if (rhs.empty()) {
    return lhs;
  }
  std::string a = lhs.native();
  const std::string& b = rhs.native();
  if (a.back() != '/' && a.back() != '\\') {
    a.push_back('/');
  }
  if (!b.empty() && (b.front() == '/' || b.front() == '\\')) {
    a.append(b, 1, std::string::npos);
  } else {
    a.append(b);
  }
  return path{std::move(a)};
}

}  // namespace pirate

#endif
