#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "make_archive.h"
#include "pirate/archive_iterator.h"
#include "pirate/extract_factory.h"
#include "pirate/archive.h"

namespace fs = std::filesystem;

class ArchiveTest : public ::testing::Test {
 protected:
  void SetUp() override {
    root_ = fs::temp_directory_path() / "pirate_gtest";
    fs::remove_all(root_);
    fs::create_directories(root_);
  }
  void TearDown() override { fs::remove_all(root_); }

  fs::path root_;
};

TEST_F(ArchiveTest, RejectsNonAsciiPath) {
  EXPECT_THROW(pirate::archive("C:/tmp/\xE4\xB8\xAD.zip"),
               std::runtime_error);
}

TEST_F(ArchiveTest, OpenMissingFile) {
  pirate::archive archive((root_ / "missing.zip").string());
  EXPECT_FALSE(static_cast<bool>(archive));
  auto it = archive.begin();
  EXPECT_TRUE(it.at_end());
  EXPECT_TRUE(it == pirate::archive_sentinel{});
}

TEST_F(ArchiveTest, EmptyZipListAndExtract) {
  const auto zip = root_ / "empty.zip";
  pirate_test::write_bytes(zip, pirate_test::make_store_zip({}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  EXPECT_EQ(archive.location().string(), zip.string());
  int n = 0;
  for (const auto& e : archive) {
    (void)e;
    ++n;
  }
  EXPECT_EQ(n, 0);
  EXPECT_EQ(archive.extract(std::vector<pirate::archive_entry>{},
                            pirate::path{(root_ / "out").string()}),
            0);
}

TEST_F(ArchiveTest, ZipListExtractAndFactories) {
  const auto zip = root_ / "sample.zip";
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip({
               {"dir/", "", true},
               {"dir/a.txt", "alpha", false},
               {"dir/b.txt", "bravo", false},
               {"backslash\\", "", true},
           }));

  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);

  std::vector<pirate::archive_entry> all;
  std::vector<pirate::archive_entry> files;
  for (const auto& e : archive) {
    EXPECT_TRUE(e.has_index());
    EXPECT_EQ(e.compressed_size(), 0u);
    EXPECT_EQ(e.is_regular_file(), !e.is_directory());
    all.push_back(e);
    if (e.is_regular_file()) {
      EXPECT_GT(e.file_size(), 0u);
      files.push_back(e);
    }
  }
  ASSERT_GE(all.size(), 3u);
  ASSERT_EQ(files.size(), 2u);

  pirate::archive_iterator def;
  EXPECT_TRUE(def.at_end());
  EXPECT_TRUE(pirate::archive_sentinel{} == def);
  EXPECT_FALSE(def != pirate::archive_sentinel{});
  def++;
  EXPECT_TRUE(def.at_end());

  auto it = archive.begin();
  ASSERT_FALSE(it.at_end());
  EXPECT_EQ(it->index(), 0u);
  ++it;

  const auto out = root_ / "out";
  EXPECT_EQ(archive.extract(files[0], pirate::path{out.string()}), 0);
  std::ifstream in(out / "dir" / "a.txt");
  std::string body((std::istreambuf_iterator<char>(in)), {});
  EXPECT_EQ(body, "alpha");

  const auto out2 = root_ / "out2";
  EXPECT_EQ(pirate::extract(archive, files, pirate::path{out2.string()}), 0);
  EXPECT_TRUE(fs::exists(out2 / "dir" / "b.txt"));

  const auto out3 = root_ / "out3";
  pirate::extract_options opts;
  opts.concurrency = 4;
  EXPECT_EQ(archive.extract<pirate::parallel_extract_factory>(
                files, pirate::path{out3.string()}, opts),
            0);

  pirate::archive_entry empty;
  EXPECT_FALSE(empty.has_index());
  EXPECT_EQ(archive.extract(empty, pirate::path{(root_ / "x").string()}), -1);
}

TEST_F(ArchiveTest, ZipSlipRejected) {
  const auto zip = root_ / "slip.zip";
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip({{"../evil.txt", "nope", false}}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  std::vector<pirate::archive_entry> files;
  for (const auto& e : archive) {
    if (e.is_regular_file()) {
      files.push_back(e);
    }
  }
  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(archive.extract(files, pirate::path{(root_ / "safe").string()}),
            -1);
}

TEST_F(ArchiveTest, ExtractSubsetAndNoCreateDirs) {
  const auto zip = root_ / "multi.zip";
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip(
               {{"keep.txt", "k", false}, {"skip.txt", "s", false}}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  std::vector<pirate::archive_entry> files;
  for (const auto& e : archive) {
    files.push_back(e);
  }
  ASSERT_EQ(files.size(), 2u);
  const auto out = root_ / "subset";
  EXPECT_EQ(archive.extract(std::vector<pirate::archive_entry>{files[0]},
                            pirate::path{out.string()}),
            0);
  EXPECT_TRUE(fs::exists(out / "keep.txt"));
  EXPECT_FALSE(fs::exists(out / "skip.txt"));

  pirate::extract_options opts;
  opts.create_directories = false;
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip({{"nested/x.txt", "n", false}}));
  pirate::archive nested(zip.string());
  std::vector<pirate::archive_entry> nested_files;
  for (const auto& e : nested) {
    nested_files.push_back(e);
  }
  EXPECT_EQ(nested.extract(nested_files, pirate::path{(root_ / "nodir").string()},
                           opts),
            -1);
}

TEST_F(ArchiveTest, WantedIndexMissing) {
  const auto zip = root_ / "one.zip";
  pirate_test::write_bytes(zip,
                           pirate_test::make_store_zip({{"a.txt", "a", false}}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  auto fake = pirate::archive_entry::with_index(99, pirate::path{"a.txt"});
  EXPECT_EQ(archive.extract(fake, pirate::path{(root_ / "d").string()}), -1);
}

TEST_F(ArchiveTest, ExtractUnopenedAndCreateDirFails) {
  pirate::archive missing((root_ / "nope.zip").string());
  EXPECT_EQ(missing.extract(pirate::archive_entry::with_index(0),
                            pirate::path{(root_ / "x").string()}),
            -1);

  const auto zip = root_ / "nested.zip";
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip({{"nested/x.txt", "n", false}}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  std::vector<pirate::archive_entry> files;
  for (const auto& e : archive) {
    files.push_back(e);
  }
  const auto blocker = root_ / "blocker";
  pirate_test::write_bytes(blocker, "not-a-dir");
  EXPECT_EQ(archive.extract(files, pirate::path{blocker.string()}), -1);
}

TEST_F(ArchiveTest, ExtractSecondFileAndDirectory) {
  const auto zip = root_ / "two.zip";
  pirate_test::write_bytes(
      zip, pirate_test::make_store_zip({{"first.txt", "1", false},
                                        {"dir/", "", true},
                                        {"dir/second.txt", "2", false}}));
  pirate::archive archive(zip.string());
  ASSERT_TRUE(archive);
  std::vector<pirate::archive_entry> all;
  for (const auto& e : archive) {
    all.push_back(e);
  }
  ASSERT_EQ(all.size(), 3u);
  const auto out = root_ / "skip";
  EXPECT_EQ(archive.extract(std::vector<pirate::archive_entry>{all[2]},
                            pirate::path{out.string()}),
            0);
  EXPECT_FALSE(fs::exists(out / "first.txt"));
  EXPECT_TRUE(fs::exists(out / "dir" / "second.txt"));
  EXPECT_EQ(archive.extract(std::vector<pirate::archive_entry>{all[1]},
                            pirate::path{(root_ / "dirs").string()}),
            0);
}

TEST_F(ArchiveTest, CorruptArchive) {
  const auto zip = root_ / "bad.zip";
  pirate_test::write_bytes(zip, "not-an-archive");
  pirate::archive archive(zip.string());
  EXPECT_FALSE(archive);
  int n = 0;
  for (const auto& e : archive) {
    (void)e;
    ++n;
  }
  EXPECT_EQ(n, 0);
}

TEST_F(ArchiveTest, MoveAndBeginAfterDelete) {
  const auto zip = root_ / "mv.zip";
  pirate_test::write_bytes(zip,
                           pirate_test::make_store_zip({{"a.txt", "a", false}}));
  pirate::archive a(zip.string());
  ASSERT_TRUE(a);
  pirate::archive b{std::move(a)};
  EXPECT_FALSE(a);
  EXPECT_TRUE(b);
  pirate::archive c(zip.string());
  c = std::move(b);
  EXPECT_TRUE(c);
  c = std::move(c);
  EXPECT_TRUE(c);
  {
    pirate::archive closed{std::move(c)};
  }
  fs::remove(zip);
  pirate::archive gone(zip.string());
  EXPECT_FALSE(gone);
  auto it = gone.begin();
  EXPECT_TRUE(it.at_end());
}

TEST_F(ArchiveTest, TarAndTarGz) {
  const auto tar = root_ / "sample.tar";
  const auto tgz = root_ / "sample.tar.gz";
  const auto bytes = pirate_test::make_ustar(
      {{"hello.txt", "hello tar", false}, {"sub/", "", true}});
  pirate_test::write_bytes(tar, bytes);
  pirate_test::write_bytes(tgz, pirate_test::gzip_wrap(bytes));

  pirate::archive t(tar.string());
  ASSERT_TRUE(t);
  int n = 0;
  std::vector<pirate::archive_entry> files;
  for (const auto& e : t) {
    ++n;
    if (e.is_regular_file()) {
      files.push_back(e);
    }
  }
  EXPECT_GE(n, 1);
  EXPECT_EQ(t.extract(files, pirate::path{(root_ / "tarout").string()}), 0);
  std::ifstream in(root_ / "tarout" / "hello.txt");
  std::string body((std::istreambuf_iterator<char>(in)), {});
  EXPECT_EQ(body, "hello tar");

  pirate::archive g(tgz.string());
  ASSERT_TRUE(g);
  int gn = 0;
  for (const auto& e : g) {
    (void)e;
    ++gn;
  }
  EXPECT_GE(gn, 1);
}

TEST_F(ArchiveTest, IteratorSentinelBothWays) {
  pirate::archive_iterator it;
  pirate::archive_sentinel s;
  EXPECT_TRUE(it == s);
  EXPECT_TRUE(s == it);
  EXPECT_FALSE(it != s);
  EXPECT_FALSE(s != it);
}
