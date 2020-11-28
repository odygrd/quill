#include "doctest/doctest.h"

#include "quill/detail/spsc_queue/BoundedSPSCEventQueue.h"
#include <cstring>
#include <thread>
#include <vector>

TEST_SUITE_BEGIN("BoundedSPSCEventQueue");

using namespace quill::detail;

struct test_struct_ints
{
  size_t size() const noexcept { return sizeof(*this); }
  uint32_t x;
  uint32_t y;
};

struct test_struct_ints_2
{
  size_t size() const noexcept { return sizeof(*this); }
  uint64_t x;
  uint64_t y;
};

TEST_CASE("produce_consume_many_same_type")
{
  BoundedSPSCEventQueue<test_struct_ints> buffer;

  REQUIRE_EQ(buffer.empty(), true);

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 170; ++i)
    {
      QUILL_MAYBE_UNUSED auto res = buffer.try_emplace<test_struct_ints>(i, i);
    }

    REQUIRE_EQ(buffer.empty(), false);

    // first observe the first value without removing
    for (uint32_t i = 0; i < 10; ++i)
    {
      auto handl = buffer.try_pop();

      REQUIRE_EQ(handl.data()->x, 0);
      REQUIRE_EQ(handl.data()->y, 0);
      handl.release();
    }

    // now read all the same values but this time also remove them
    for (uint32_t i = 0; i < 170; ++i)
    {
      auto handl = buffer.try_pop();

      REQUIRE_EQ(handl.data()->x, i);
      REQUIRE_EQ(handl.data()->y, i);
    }

    REQUIRE_EQ(buffer.empty(), true);
  }

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 85; ++i)
    {
      QUILL_MAYBE_UNUSED auto res = buffer.try_emplace<test_struct_ints>(i + 100, i + 100);
    }

    REQUIRE_EQ(buffer.empty(), false);

    // first observe the first value without removing
    for (uint32_t i = 0; i < 10; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->x, 0 + 100);
      REQUIRE_EQ(handl.data()->y, 0 + 100);
      handl.release();
    }

    // now read the all the values but this time also remove them
    for (uint32_t i = 0; i < 85; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->x, i + 100);
      REQUIRE_EQ(handl.data()->y, i + 100);
    }

    REQUIRE_EQ(buffer.empty(), true);
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

TEST_CASE("produce_consume_many_different_types")
{
  BoundedSPSCEventQueue<TestBase> buffer;

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 43; ++i)
    {
      QUILL_MAYBE_UNUSED auto res = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));
    }

    for (uint32_t i = 0; i < 43; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->get_x(), 0);
      REQUIRE_EQ(handl.data()->get_y(), 0);
      REQUIRE_EQ(handl.data()->get_z(), i);
      REQUIRE_EQ(handl.data()->get_vec().size(), 12);
    }
  }

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 92; ++i)
    {
      QUILL_MAYBE_UNUSED auto res =
        buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
    }

    for (uint32_t i = 0; i < 92; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->get_x(), i + 10);
      REQUIRE_EQ(handl.data()->get_y(), i + 20);
      REQUIRE_EQ(handl.data()->get_z(), i);
    }
  }

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 43; ++i)
    {
      QUILL_MAYBE_UNUSED auto res = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));
    }

    for (uint32_t i = 0; i < 43; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->get_x(), 0);
      REQUIRE_EQ(handl.data()->get_y(), 0);
      REQUIRE_EQ(handl.data()->get_z(), i);
    }
  }

  for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
  {
    for (uint32_t i = 0; i < 92; ++i)
    {
      QUILL_MAYBE_UNUSED auto res =
        buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
    }

    for (uint32_t i = 0; i < 92; ++i)
    {
      auto handl = buffer.try_pop();
      REQUIRE_EQ(handl.data()->get_x(), i + 10);
      REQUIRE_EQ(handl.data()->get_y(), i + 20);
      REQUIRE_EQ(handl.data()->get_z(), i);
    }
  }
}

TEST_CASE("produce_consume_many_multithreaded")
{
  BoundedSPSCEventQueue<TestBase> buffer;

  std::thread producer_thread([&buffer]() {
    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 43; ++i)
      {
        auto pushed = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));

        while (!pushed)
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          pushed = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));
        }
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 92; ++i)
      {
        auto pushed = buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
        while (!pushed)
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          pushed = buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
        }
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 43; ++i)
      {
        auto pushed = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));

        while (!pushed)
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          pushed = buffer.try_emplace<TestDerivedSmall>(static_cast<double>(i));
        }
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 92; ++i)
      {
        auto pushed = buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
        while (!pushed)
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          pushed = buffer.try_emplace<TestDerivedLarge>(i + 10, i + 20, static_cast<double>(i));
        }
      }
    }
  });

  std::thread consumer_thread([&buffer]() {
    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 43; ++i)
      {
        auto handl = buffer.try_pop();

        while (!handl.is_valid())
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          handl = buffer.try_pop();
        }

        REQUIRE_EQ(handl.data()->get_x(), 0);
        REQUIRE_EQ(handl.data()->get_y(), 0);
        REQUIRE_EQ(handl.data()->get_z(), i);
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 92; ++i)
      {
        auto handl = buffer.try_pop();

        while (!handl.is_valid())
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          handl = buffer.try_pop();
        }

        REQUIRE_EQ(handl.data()->get_x(), i + 10);
        REQUIRE_EQ(handl.data()->get_y(), i + 20);
        REQUIRE_EQ(handl.data()->get_z(), i);
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 43; ++i)
      {
        auto handl = buffer.try_pop();

        while (!handl.is_valid())
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          handl = buffer.try_pop();
        }

        REQUIRE_EQ(handl.data()->get_x(), 0);
        REQUIRE_EQ(handl.data()->get_y(), 0);
        REQUIRE_EQ(handl.data()->get_z(), i);
      }
    }

    for (int wrap_cnt = 0; wrap_cnt < 10; ++wrap_cnt)
    {
      for (uint32_t i = 0; i < 92; ++i)
      {
        auto handl = buffer.try_pop();

        while (!handl.is_valid())
        {
          std::this_thread::sleep_for(std::chrono::microseconds{2});
          handl = buffer.try_pop();
        }

        REQUIRE_EQ(handl.data()->get_x(), i + 10);
        REQUIRE_EQ(handl.data()->get_y(), i + 20);
        REQUIRE_EQ(handl.data()->get_z(), i);
      }
    }
  });

  producer_thread.join();
  consumer_thread.join();
}

TEST_SUITE_END();