#include <gtest/gtest.h>

#include "pirate/path.h"

TEST(Path, DefaultAndNull) {
  pirate::path empty;
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.string(), "");
  pirate::path from_null{static_cast<const char*>(nullptr)};
  EXPECT_TRUE(from_null.empty());
}

TEST(Path, FilenameAndParent) {
  pirate::path plain{"readme"};
  EXPECT_EQ(plain.filename().string(), "readme");
  EXPECT_TRUE(plain.parent().empty());

  pirate::path unix{"a/b/c.txt"};
  EXPECT_EQ(unix.filename().string(), "c.txt");
  EXPECT_EQ(unix.parent().string(), "a/b");

  pirate::path win{"a\\b\\c.txt"};
  EXPECT_EQ(win.filename().string(), "c.txt");
  EXPECT_EQ(win.parent().string(), "a\\b");

  pirate::path root_child{"/x"};
  EXPECT_TRUE(root_child.parent().empty());
}

TEST(Path, Join) {
  pirate::path empty;
  pirate::path rhs{"b"};
  EXPECT_EQ((empty / rhs).string(), "b");
  EXPECT_EQ((rhs / empty).string(), "b");
  EXPECT_EQ((pirate::path{"a"} / pirate::path{"b"}).string(), "a/b");
  EXPECT_EQ((pirate::path{"a/"} / pirate::path{"b"}).string(), "a/b");
  EXPECT_EQ((pirate::path{"a\\"} / pirate::path{"b"}).string(), "a\\b");
  EXPECT_EQ((pirate::path{"a"} / pirate::path{"/b"}).string(), "a/b");
  EXPECT_EQ((pirate::path{"a"} / pirate::path{"\\b"}).string(), "a/b");
  EXPECT_EQ(pirate::path{std::string{"z"}}.native(), "z");
}
