#include "doctest/doctest.h"

#include "quill/detail/misc/Utilities.h"
#include "quill/detail/spsc_queue/UnboundedQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("UnboundedQueue");

using namespace quill::detail;

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
  UnboundedQueue buffer;

  std::thread producer_thread(
    [&buffer]()
    {
      for (int repeat_cnt = 0; repeat_cnt < 10; ++repeat_cnt)
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

  // Delay creating the consumer thread
  std::this_thread::sleep_for(std::chrono::milliseconds{300});

  std::thread consumer_thread(
    [&buffer]()
    {
      for (int repeat_cnt = 0; repeat_cnt < 10; ++repeat_cnt)
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