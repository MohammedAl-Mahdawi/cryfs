#include "../../../../implementations/ondisk/OnDiskBlock.h"
#include "google/gtest/gtest.h"

#include <messmer/cpp-utils/tempfile/TempFile.h>
#include <messmer/cpp-utils/tempfile/TempDir.h>

using ::testing::Test;
using ::testing::WithParamInterface;
using ::testing::Values;
using cpputils::Data;
using cpputils::TempFile;
using cpputils::TempDir;
using cpputils::unique_ref;

using namespace blockstore;
using namespace blockstore::ondisk;

namespace bf = boost::filesystem;

class OnDiskBlockCreateTest: public Test {
public:
  OnDiskBlockCreateTest()
  // Don't create the temp file yet (therefore pass false to the TempFile constructor)
  : dir(),
    key(Key::FromString("1491BB4932A389EE14BC7090AC772972")),
    file(dir.path() / key.ToString(), false) {
  }
  TempDir dir;
  Key key;
  TempFile file;
};

TEST_F(OnDiskBlockCreateTest, CreatingBlockCreatesFile) {
  EXPECT_FALSE(bf::exists(file.path()));

  auto block = OnDiskBlock::CreateOnDisk(dir.path(), key, Data(0));

  EXPECT_TRUE(bf::exists(file.path()));
  EXPECT_TRUE(bf::is_regular_file(file.path()));
}

TEST_F(OnDiskBlockCreateTest, CreatingExistingBlockReturnsNull) {
  auto block1 = OnDiskBlock::CreateOnDisk(dir.path(), key, Data(0));
  auto block2 = OnDiskBlock::CreateOnDisk(dir.path(), key, Data(0));
  EXPECT_TRUE((bool)block1);
  EXPECT_FALSE((bool)block2);
}

class OnDiskBlockCreateSizeTest: public OnDiskBlockCreateTest, public WithParamInterface<size_t> {
public:
  unique_ref<OnDiskBlock> block;
  Data ZEROES;

  OnDiskBlockCreateSizeTest():
    block(OnDiskBlock::CreateOnDisk(dir.path(), key, std::move(Data(GetParam()).FillWithZeroes())).value()),
    ZEROES(block->size())
  {
    ZEROES.FillWithZeroes();
  }
};
INSTANTIATE_TEST_CASE_P(OnDiskBlockCreateSizeTest, OnDiskBlockCreateSizeTest, Values(0, 1, 5, 1024, 10*1024*1024));

TEST_P(OnDiskBlockCreateSizeTest, OnDiskSizeIsCorrect) {
  Data fileContent = Data::LoadFromFile(file.path()).value();
  EXPECT_EQ(GetParam(), fileContent.size());
}

TEST_P(OnDiskBlockCreateSizeTest, OnDiskBlockIsZeroedOut) {
  Data fileContent = Data::LoadFromFile(file.path()).value();
  EXPECT_EQ(ZEROES, fileContent);
}

// This test is also tested by OnDiskBlockStoreTest, but there the block is created using the BlockStore interface.
// Here, we create it using OnDiskBlock::CreateOnDisk()
TEST_P(OnDiskBlockCreateSizeTest, InMemorySizeIsCorrect) {
  EXPECT_EQ(GetParam(), block->size());
}

// This test is also tested by OnDiskBlockStoreTest, but there the block is created using the BlockStore interface.
// Here, we create it using OnDiskBlock::CreateOnDisk()
TEST_P(OnDiskBlockCreateSizeTest, InMemoryBlockIsZeroedOut) {
  EXPECT_EQ(0, std::memcmp(ZEROES.data(), block->data(), block->size()));
}