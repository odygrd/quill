#include <iostream>
#include <string>

#include "quill/Quill.h"
#include <chrono>
#include <cstdint>
#include <limits>
#include <random>
#include <thread>
#include <algorithm>
#include <numeric>

template <typename T>
std::pair<std::string, std::vector<double>> log_numeric_performance(int amount, std::string name);
std::pair<std::string, std::vector<double>> log_double_performance(int amount, std::string name);
std::pair<std::string, std::vector<double>> log_double_and_long_int_performance(int amount, std::string name);
std::pair<std::string, std::vector<double>> log_string_performance(int iterations,
                                                                   int min_len,
                                                                   int max_len,
                                                                   std::string name);
std::pair<std::string, std::vector<double>> log_multiple_string_performance(int iterations,
                                                                            int min_len,
                                                                            int max_len,
                                                                            std::string name);
std::pair<std::string, std::vector<double>> log_multiple_string_and_int_performance(int iterations,
                                                                                    int min_len,
                                                                                    int max_len,
                                                                                    std::string name);

template <typename T>
std::pair<std::string, std::vector<double>> log_numeric_performance(int amount, std::string name)
{
  name += "/iterations:" + std::to_string(amount);

  std::vector<T> stored_keys;

  std::mt19937 rng{std::random_device()()};
  std::uniform_int_distribution<T> distribution(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

  // generate a vector of random ints
  for (int i = 0; i < amount; ++i)
  {
    stored_keys.push_back(distribution(rng));
  }

  std::vector<double> results;

  quill::Logger* logger = quill::get_logger("bench");
  for (auto elem : stored_keys)
  {
    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger, "This is an int [{}]", elem);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

std::pair<std::string, std::vector<double>> log_double_performance(int amount, std::string name)
{
  name += "/iterations:" + std::to_string(amount);

  std::vector<double> stored_keys;

  std::mt19937 rng{std::random_device()()};
  std::uniform_real_distribution<double> distribution(std::numeric_limits<double>::min(),
                                                      std::numeric_limits<double>::max());

  // generate a vector of random ints
  for (int i = 0; i < amount; ++i)
  {
    stored_keys.push_back(distribution(rng));
  }

  std::vector<double> results;

  quill::Logger* logger = quill::get_logger("bench");
  for (auto elem : stored_keys)
  {
    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger, "This is an double [{}]", elem);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

std::pair<std::string, std::vector<double>> log_double_and_long_int_performance(int amount, std::string name)
{
  name += "/iterations:" + std::to_string(amount);

  std::vector<double> stored_keys_doubles;
  std::vector<int64_t> stored_keys_ints;

  std::mt19937 rng{std::random_device()()};
  std::uniform_real_distribution<double> distribution(std::numeric_limits<double>::min(),
                                                      std::numeric_limits<double>::max());

  // generate a vector of random ints
  for (int i = 0; i < amount; ++i)
  {
    stored_keys_ints.push_back(static_cast<int64_t>(distribution(rng)));
    stored_keys_doubles.push_back(distribution(rng));
  }

  std::vector<double> results;

  quill::Logger* logger = quill::get_logger("bench");
  for (size_t i = 0; i < stored_keys_ints.size(); i = i + 5)
  {
    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger,
             "This is an double and an int and a double and an int and a double and an int and a "
             "double and an int and a double and an int [{}, {}, {}, {}, {}, "
             "{}, {}, {}, {}, {}]",
             stored_keys_doubles[i], stored_keys_ints[i], stored_keys_doubles[i + 1],
             stored_keys_ints[i + 1], stored_keys_doubles[i + 2], stored_keys_ints[i + 2],
             stored_keys_doubles[i + 3], stored_keys_ints[i + 3], stored_keys_doubles[i + 4],
             stored_keys_ints[i + 4]);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

/**
 * Generate a random string
 * @param min_len
 * @param max_len
 * @return
 */
inline std::string get_random_string(int min_len, int max_len)
{
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  // Keep engine as static else we might get similar results
  static std::mt19937 rng{std::random_device()()};
  std::uniform_int_distribution<int> distribution(0, (sizeof(alphanum) - 1));

  auto const random_num = distribution(rng);

  int const len = (random_num % (max_len - min_len)) + min_len;

  std::string result;
  for (int i = 0; i < len; ++i)
  {
    result += alphanum[distribution(rng)];
  }
  return result;
}

std::pair<std::string, std::vector<double>> log_string_performance(int iterations, int min_len, int max_len, std::string name)
{
  name += "/iterations:" + std::to_string(iterations) + "/min_len:" + std::to_string(min_len) +
    "/max_len:" + std::to_string(max_len);

  std::vector<std::string> stored_keys;

  // generate a vector of random ints
  for (int i = 0; i < iterations; ++i)
  {
    stored_keys.push_back(get_random_string(min_len, max_len));
  }

  std::vector<double> results;

  quill::Logger* logger = quill::get_logger("bench");

  for (auto const& elem : stored_keys)
  {
    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger, "This is an string [{}]", elem);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

std::pair<std::string, std::vector<double>> log_multiple_string_performance(int iterations,
                                                                            int min_len,
                                                                            int max_len,
                                                                            std::string name)
{
  // log multiple strings in the same log message

  name += "/iterations:" + std::to_string(iterations) + "/min_len:" + std::to_string(min_len) +
    " * 3/max_len:" + std::to_string(max_len) + " * 3";

  std::vector<std::string> stored_keys;

  // generate a vector of random ints
  for (int i = 0; i < iterations * 3; ++i)
  {
    stored_keys.push_back(get_random_string(min_len, max_len));
  }

  std::vector<double> results;

  for (int i = 0; i < stored_keys.size(); i += 3)
  {
    if (i + 2 >= stored_keys.size())
    {
      break;
    }

    quill::Logger* logger = quill::get_logger("bench");

    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger, "This is an string and another string and another string [{}{}{}]",
             stored_keys[i], stored_keys[i + 1], stored_keys[i + 2]);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

std::pair<std::string, std::vector<double>> log_multiple_string_and_int_performance(int iterations,
                                                                                    int min_len,
                                                                                    int max_len,
                                                                                    std::string name)
{
  // log multiple strings in the same log message

  name += "/iterations:" + std::to_string(iterations) + "/min_len:" + std::to_string(min_len) +
    " * 2/max_len:" + std::to_string(max_len) + " * 2";

  std::vector<std::string> stored_strings;
  // generate a vector of random strings
  for (int i = 0; i < iterations * 2; ++i)
  {
    stored_strings.push_back(get_random_string(min_len, max_len));
  }

  std::vector<std::int32_t> stored_ints;
  std::mt19937 rng{std::random_device()()};
  std::uniform_int_distribution<std::int32_t> distribution(
    std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max());

  // generate a vector of random ints
  for (int i = 0; i < iterations * 2; ++i)
  {
    stored_ints.push_back(distribution(rng));
  }

  std::vector<double> results;

  for (int i = 0; i < stored_strings.size(); i += 2)
  {
    if (i + 1 >= stored_strings.size())
    {
      break;
    }

    quill::Logger* logger = quill::get_logger("bench");

    auto const start = std::chrono::high_resolution_clock::now();
    LOG_INFO(logger, "This is an string and an int and another string [{}{}{}]", stored_strings[i],
             stored_ints[i], stored_strings[i + 1], stored_ints[i + 1]);
    auto const end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    results.push_back(duration);
  }
  return std::make_pair(name, results);
}

int main()
{
  // Set this thread affinity
//
//  cpu_set_t cpuset;
//  CPU_ZERO(&cpuset);
//  CPU_SET(1, &cpuset);
//
//  auto const err = sched_setaffinity(0, sizeof(cpuset), &cpuset);

//  if (QUILL_UNLIKELY(err == -1))
//  {
//    throw std::system_error((errno), std::generic_category());
//  }

  // Logging
  quill::config::set_backend_thread_cpu_affinity(0);
  quill::config::set_backend_thread_sleep_duration(std::chrono::nanoseconds{0});

  auto file_handler = quill::file_handler("bench_log", "w");
  quill::Logger* logger = quill::create_logger("bench", file_handler);

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::Info);

  quill::start();

  // let the logger start
  std::this_thread::sleep_for(std::chrono::seconds(3));

  std::vector<std::pair<std::string, std::vector<double>>> results;

  auto const start = std::chrono::high_resolution_clock::now();
  // Int bench
  {
    auto res = log_numeric_performance<std::uint32_t>(100'000, "log one uint32_t");
    results.push_back(res);
  }

  {
    auto res = log_double_performance(100'000, "log one double");
    results.push_back(res);
  }

  {
    auto res = log_double_and_long_int_performance(100'000, "log five doubles and five ints");
    results.push_back(res);
  }

  {
    auto res = log_string_performance(100'000, 6, 23, "log short string");
    results.push_back(res);
  }

  //  {
  //    auto res = log_string_performance(100'000, 10, 22, "log_string");
  //    results.push_back(res);
  //  }
  //
  //  {
  //    auto res = log_multiple_string_performance(100'000, 100, 200, "log_multiple_string");
  //    results.push_back(res);
  //  }
  //
  //  {
  //    auto res = log_multiple_string_and_int_performance(100'000, 100, 200,
  //    "log_multiple_string_and_int"); results.push_back(res);
  //  }

  quill::flush();
  auto const end = std::chrono::high_resolution_clock::now();

  auto const total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "***--- Results --- ***" << std::endl;
  std::cout << "Total Duration: " << total_duration << std::endl;

  for (auto const& result : results)
  {
    auto vec_results = result.second;
    auto test_name = result.first;

    std::sort(vec_results.begin(), vec_results.end(), std::less<>());

    //
    double median;
    if (vec_results.size() % 2 == 0)
    {
      median = (vec_results[vec_results.size() / 2 - 1] + vec_results[vec_results.size() / 2]) / 2;
    }
    else
    {
      median = vec_results[vec_results.size() / 2];
    }

    //
    double mean = std::accumulate(vec_results.begin(), vec_results.end(), 0.0) / vec_results.size();

    //
    double percentile_10 = vec_results[static_cast<size_t>(vec_results.size() * 0.1)];
    double percentile_20 = vec_results[static_cast<size_t>(vec_results.size() * 0.2)];
    double percentile_30 = vec_results[static_cast<size_t>(vec_results.size() * 0.3)];
    double percentile_50 = vec_results[static_cast<size_t>(vec_results.size() * 0.5)];
    double percentile_75 = vec_results[static_cast<size_t>(vec_results.size() * 0.75)];
    double percentile_90 = vec_results[static_cast<size_t>(vec_results.size() * 0.9)];
    double percentile_95 = vec_results[static_cast<size_t>(vec_results.size() * 0.95)];
    double percentile_99 = vec_results[static_cast<size_t>(vec_results.size() * 0.99)];

    std::cout << std::endl;
    std::cout << "---" << test_name << "---" << std::endl;
    std::cout << "min " << vec_results.front() << " ns" << std::endl;
    std::cout << "mean " << mean << " ns" << std::endl;
    std::cout << "median " << median << " ns" << std::endl;
    std::cout << "10th percentile " << percentile_10 << " ns" << std::endl;
    std::cout << "20th percentile " << percentile_20 << " ns" << std::endl;
    std::cout << "30th percentile " << percentile_30 << " ns" << std::endl;
    std::cout << "50th percentile " << percentile_50 << " ns" << std::endl;
    std::cout << "75th percentile " << percentile_75 << " ns" << std::endl;
    std::cout << "90th percentile " << percentile_90 << " ns" << std::endl;
    std::cout << "95th percentile " << percentile_95 << " ns" << std::endl;
    std::cout << "99th percentile " << percentile_99 << " ns" << std::endl;
  }

  return 0;
}