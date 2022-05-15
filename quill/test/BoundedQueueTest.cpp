#include "doctest/doctest.h"

#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/BoundedQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedQueue");

using namespace quill::detail;

TEST_CASE("read_write_multithreaded_plain_ints")
{
  BoundedQueue buffer;

  std::thread producer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          auto* write_buffer = buffer.prepare_write(sizeof(uint32_t));

          while (!write_buffer)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            write_buffer = buffer.prepare_write(sizeof(uint32_t));
          }

          std::memcpy(write_buffer, &i, sizeof(uint32_t));
          buffer.commit_write(sizeof(uint32_t));
        }
      }
    });

  std::thread consumer_thread(
    [&buffer]()
    {
      for (uint32_t wrap_cnt = 0; wrap_cnt < 20; ++wrap_cnt)
      {
        for (uint32_t i = 0; i < 8192; ++i)
        {
          auto [read_buffer, bytes] = buffer.prepare_read();
          while (bytes == 0)
          {
            std::this_thread::sleep_for(std::chrono::microseconds{2});
            std::tie(read_buffer, bytes) = buffer.prepare_read();
          }

          auto value = reinterpret_cast<uint32_t const*>(read_buffer);
          REQUIRE_EQ(*value, i);
          buffer.finish_read(sizeof(uint32_t));
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();
}

struct test_struct_32
{
  explicit test_struct_32(uint32_t inx) : x1(inx) {}
  uint32_t x1{123};
};

struct test_struct_96
{
  explicit test_struct_96(uint32_t inx) : x2(inx), y2(inx * 10) {}
  uint64_t x2{456};
  uint32_t y2{789};
};

TEST_CASE("produce_consume_many_of_the_same_type")
{
  BoundedQueue buffer;

  REQUIRE_EQ(buffer.empty(), true);

  /**
   * Emplace struct 32
   */
  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 170; ++i)
    {
      // figure out the object size - including the cache line padding

      // we want to see where at the buffer we will end up if we inserted the object
      std::byte* end_pos = buffer.producer_pos() + sizeof(uint32_t) + sizeof(test_struct_32);

      // Then we align that pointer to the next cache line
      std::byte const* end_pos_aligned =
        quill::detail::align_pointer<quill::detail::CACHELINE_SIZE, std::byte>(end_pos);

      assert((end_pos_aligned > buffer.producer_pos()) &&
             "end_pos_aligned must be greater than _producer_pos");

      // calculate the object size including padding excluding the bytes we use to store the size of the object
      auto const total_size = static_cast<uint32_t>(end_pos_aligned - buffer.producer_pos());
      uint32_t const obj_size_inc_padding = total_size - sizeof(uint32_t);

      auto* write_buffer = buffer.prepare_write(total_size);

      while (!write_buffer)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        write_buffer = buffer.prepare_write(sizeof(uint32_t));
      }

      // first write the size of the object
      std::memcpy(write_buffer, &obj_size_inc_padding, sizeof(obj_size_inc_padding));
      write_buffer += sizeof(obj_size_inc_padding);

      // emplace construct the object there
      new (write_buffer) test_struct_32{i};

      // update the buffer with the write
      buffer.commit_write(total_size);
    }

    REQUIRE_EQ(buffer.empty(), false);

    // now read all the same values but this time also remove them
    for (uint32_t i = 0; i < 170; ++i)
    {
      auto [read_buffer, bytes] = buffer.prepare_read();
      while (bytes == 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        std::tie(read_buffer, bytes) = buffer.prepare_read();
      }

      // Check that the object we read starts in a new cache line
      auto b = read_buffer;
      REQUIRE_EQ(reinterpret_cast<uintptr_t>(b) % quill::detail::CACHELINE_SIZE, 0);

      // read the size of the object
      auto obj_size = reinterpret_cast<uint32_t const*>(read_buffer);

      auto value = reinterpret_cast<test_struct_32 const*>(read_buffer + sizeof(uint32_t));
      REQUIRE_EQ(value->x1, i);

      buffer.finish_read(*obj_size + sizeof(uint32_t));
    }
  }

  /**
   * Emplace struct 96
   */
  REQUIRE_EQ(buffer.empty(), true);

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 85; ++i)
    {
      // figure out the object size - including the cache line padding

      // we want to see where at the buffer we will end up if we inserted the object
      std::byte* end_pos = buffer.producer_pos() + sizeof(uint32_t) + sizeof(test_struct_96);

      // Then we align that pointer to the next cache line
      std::byte* end_pos_aligned =
        quill::detail::align_pointer<quill::detail::CACHELINE_SIZE, std::byte>(end_pos);

      assert((end_pos_aligned > buffer.producer_pos()) &&
             "end_pos_aligned must be greater than _producer_pos");

      // calculate the object size including padding excluding the bytes we use to store the size of the object
      auto const total_size = static_cast<uint32_t>(end_pos_aligned - buffer.producer_pos());
      uint32_t const obj_size_inc_padding = total_size - sizeof(uint32_t);

      auto* write_buffer = buffer.prepare_write(total_size);

      while (!write_buffer)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        write_buffer = buffer.prepare_write(sizeof(uint32_t));
      }

      // first write the size of the object
      std::memcpy(write_buffer, &obj_size_inc_padding, sizeof(obj_size_inc_padding));
      write_buffer += sizeof(obj_size_inc_padding);

      // emplace construct the object there
      new (write_buffer) test_struct_96{i};

      // update the buffer with the write
      buffer.commit_write(total_size);
    }

    REQUIRE_EQ(buffer.empty(), false);

    // now read all the same values but this time also remove them
    for (uint32_t i = 0; i < 85; ++i)
    {
      auto [read_buffer, bytes] = buffer.prepare_read();
      while (bytes == 0)
      {
        std::this_thread::sleep_for(std::chrono::microseconds{2});
        std::tie(read_buffer, bytes) = buffer.prepare_read();
      }

      // Check that the object we read starts in a new cache line
      auto b = read_buffer;
      REQUIRE_EQ(reinterpret_cast<uintptr_t>(b) % quill::detail::CACHELINE_SIZE, 0);

      // read the size of the object
      auto obj_size = reinterpret_cast<uint32_t const*>(read_buffer);

      auto value = reinterpret_cast<test_struct_96 const*>(read_buffer + sizeof(uint32_t));
      REQUIRE_EQ(value->x2, i);
      REQUIRE_EQ(value->y2, i * 10);

      buffer.finish_read(*obj_size + sizeof(uint32_t));
    }
  }
}

class TestBase
{
public:
  virtual std::int32_t get_x() const = 0;
  virtual std::int32_t get_y() const = 0;
  virtual double get_z() const = 0;
  virtual std::vector<int> get_vec() const { return {}; }
  virtual size_t size() const noexcept = 0;
  virtual ~TestBase() = default;
};

class TestDerivedSmall : public TestBase
{
public:
  explicit TestDerivedSmall(double z) : z(z)
  {
    v.resize(10);
    v.push_back(1);
    v.push_back(2);
  }
  ~TestDerivedSmall() override = default;

  std::int32_t get_x() const override { return 0; }
  std::int32_t get_y() const override { return 0; }
  double get_z() const override { return z; }
  std::vector<int> get_vec() const override { return v; }
  size_t size() const noexcept override { return sizeof(*this); }

private:
  double z;
  std::vector<int> v;
};

class TestDerivedLarge : public TestBase
{
public:
  TestDerivedLarge(uint32_t x, uint32_t y, double z) : x(x), y(y), z(z) {}
  ~TestDerivedLarge() override = default;

  std::int32_t get_x() const override { return x; }
  std::int32_t get_y() const override { return y; }
  double get_z() const override { return z; }
  size_t size() const noexcept override { return sizeof(*this); }

private:
  uint32_t x;
  uint32_t y;
  double z;
};

TEST_CASE("produce_consume_many_multithreaded")
{
  BoundedQueue buffer;

  std::thread producer_thread(
    [&buffer]()
    {
      for (int repeat_cnt = 0; repeat_cnt < 3; ++repeat_cnt)
      {
        for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
        {
          for (uint32_t i = 0; i < 43; ++i)
          {
            // figure out the object size - including the cache line padding

            // we want to see where at the buffer we will end up if we inserted the object
            std::byte* end_pos = buffer.producer_pos() + sizeof(uint32_t) + sizeof(TestDerivedSmall);

            // Then we align that pointer to the next cache line
            std::byte* end_pos_aligned =
              quill::detail::align_pointer<quill::detail::CACHELINE_SIZE, std::byte>(end_pos);

            assert((end_pos_aligned > buffer.producer_pos()) &&
                   "end_pos_aligned must be greater than _producer_pos");

            // calculate the object size including padding excluding the bytes we use to store the size of the object
            auto const total_size = static_cast<uint32_t>(end_pos_aligned - buffer.producer_pos());
            uint32_t const obj_size_inc_padding = total_size - sizeof(uint32_t);

            auto* write_buffer = buffer.prepare_write(total_size);

            while (!write_buffer)
            {
              std::this_thread::sleep_for(std::chrono::microseconds{2});
              write_buffer = buffer.prepare_write(sizeof(uint32_t));
            }

            // first write the size of the object
            std::memcpy(write_buffer, &obj_size_inc_padding, sizeof(obj_size_inc_padding));
            write_buffer += sizeof(obj_size_inc_padding);

            // emplace construct the object there
            new (write_buffer) TestDerivedSmall{static_cast<double>(i)};

            // update the buffer with the write
            buffer.commit_write(total_size);
          }
        }

        for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
        {
          for (uint32_t i = 0; i < 92; ++i)
          {
            // figure out the object size - including the cache line padding

            // we want to see where at the buffer we will end up if we inserted the object
            std::byte* end_pos = buffer.producer_pos() + sizeof(uint32_t) + sizeof(TestDerivedLarge);

            // Then we align that pointer to the next cache line
            std::byte* end_pos_aligned =
              quill::detail::align_pointer<quill::detail::CACHELINE_SIZE, std::byte>(end_pos);

            assert((end_pos_aligned > buffer.producer_pos()) &&
                   "end_pos_aligned must be greater than _producer_pos");

            // calculate the object size including padding excluding the bytes we use to store the size of the object
            auto const total_size = static_cast<uint32_t>(end_pos_aligned - buffer.producer_pos());
            uint32_t const obj_size_inc_padding = total_size - sizeof(uint32_t);

            auto* write_buffer = buffer.prepare_write(total_size);

            while (!write_buffer)
            {
              std::this_thread::sleep_for(std::chrono::microseconds{2});
              write_buffer = buffer.prepare_write(sizeof(uint32_t));
            }

            // first write the size of the object
            std::memcpy(write_buffer, &obj_size_inc_padding, sizeof(obj_size_inc_padding));
            write_buffer += sizeof(obj_size_inc_padding);

            // emplace construct the object there
            new (write_buffer) TestDerivedLarge{i + 10, i + 20, static_cast<double>(i)};

            // update the buffer with the write
            buffer.commit_write(total_size);
          }
        }
      }
    });

  std::thread consumer_thread(
    [&buffer]()
    {
      for (int repeat_cnt = 0; repeat_cnt < 3; ++repeat_cnt)
      {
        for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
        {
          for (uint32_t i = 0; i < 43; ++i)
          {
            auto [read_buffer, bytes] = buffer.prepare_read();
            while (bytes == 0)
            {
              std::this_thread::sleep_for(std::chrono::microseconds{2});
              std::tie(read_buffer, bytes) = buffer.prepare_read();
            }

            // Check that the object we read starts in a new cache line
            auto b = read_buffer;
            REQUIRE_EQ(reinterpret_cast<uintptr_t>(b) % quill::detail::CACHELINE_SIZE, 0);

            // read the size of the object
            auto obj_size = reinterpret_cast<uint32_t const*>(read_buffer);

            auto value = reinterpret_cast<TestBase*>(read_buffer + sizeof(uint32_t));
            REQUIRE_EQ(value->get_x(), 0);
            REQUIRE_EQ(value->get_y(), 0);
            REQUIRE_EQ(value->get_z(), i);

            buffer.finish_read(*obj_size + sizeof(uint32_t));
          }
        }

        for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
        {
          for (uint32_t i = 0; i < 92; ++i)
          {
            auto [read_buffer, bytes] = buffer.prepare_read();
            while (bytes == 0)
            {
              std::this_thread::sleep_for(std::chrono::microseconds{2});
              std::tie(read_buffer, bytes) = buffer.prepare_read();
            }

            // Check that the object we read starts in a new cache line
            auto b = read_buffer;
            REQUIRE_EQ(reinterpret_cast<uintptr_t>(b) % quill::detail::CACHELINE_SIZE, 0);

            // read the size of the object
            auto obj_size = reinterpret_cast<uint32_t const*>(read_buffer);

            auto value = reinterpret_cast<TestBase*>(read_buffer + sizeof(uint32_t));
            REQUIRE_EQ(value->get_x(), i + 10);
            REQUIRE_EQ(value->get_y(), i + 20);
            REQUIRE_EQ(value->get_z(), i);

            buffer.finish_read(*obj_size + sizeof(uint32_t));
          }
        }
      }
    });

  producer_thread.join();
  consumer_thread.join();

  REQUIRE_EQ(buffer.empty(), true);
}
TEST_SUITE_END();
