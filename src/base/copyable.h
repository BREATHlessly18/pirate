#ifndef PIRATE_BASE_COPYABLE_H_
#define PIRATE_BASE_COPYABLE_H_

namespace pirate {
namespace base {

class NonCopyable {
 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

 private:
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

class Copyable {
 protected:
  Copyable(const Copyable&) = default;
  Copyable& operator=(const Copyable&) = default;
};

}  // namespace base
}  // namespace pirate

#endif  // PIRATE_BASE_COPYABLE_H_
