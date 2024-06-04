#include "TestUtilities.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace quill
{
namespace testing
{
// Convert the given file to a vector
std::vector<std::string> file_contents(fs::path const& filename)
{
  std::ifstream out_file(filename.string());

  std::vector<std::string> lines;

  for (std::string current_line; getline(out_file, current_line);)
  {
    lines.push_back(current_line);
  }

  return lines;
}

// Convert the given file to a vector
std::vector<std::wstring> wfile_contents(fs::path const& filename)
{
  std::wifstream out_file(filename.string());

  std::vector<std::wstring> lines;

  for (std::wstring current_line; getline(out_file, current_line);)
  {
    lines.push_back(current_line);
  }

  return lines;
}

// Search a vector for the given string
bool file_contains(std::vector<std::string> const& file_vector, std::string const& search_string)
{
  auto const search =
    std::find_if(file_vector.cbegin(), file_vector.cend(), [&search_string](std::string const& elem)
                 { return elem.find(search_string) != std::string::npos; });

  bool const success = search != file_vector.cend();

  if (!success)
  {
    // We failed to find and we will log for diagnostic reasons
    std::cout << "Failed to find '" << search_string << "' in:\n";
    for (auto const& line : file_vector)
    {
      std::cout << "'" << line << "'\n";
    }
  }

  return success;
}

void create_file(fs::path const& filename, std::string const& text)
{
  if (std::ofstream file(filename); file.is_open())
  {
    if (!text.empty())
    {
      file << text;
    }
    file.close();
  }
}

void remove_file(fs::path const& filename)
{
  std::error_code ec;
  fs::remove(filename, ec);
}

std::vector<std::string> gen_random_strings(size_t n, int min_len, int max_len)
{
  // Generate random strings
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist_chars(32, 126);

  // length of strings
  std::uniform_int_distribution<int> dist_len(min_len, max_len);

  // Generate a vector of random strings of dist_len
  std::vector<std::string> random_strings_vec;
  random_strings_vec.reserve(n);

  std::string result;
  for (size_t i = 0; i < n; ++i)
  {
    std::generate_n(std::back_inserter(result), dist_len(mt),
                    [&] { return static_cast<char>(dist_chars(mt)); });
    random_strings_vec.emplace_back(std::move(result));
  }
  return random_strings_vec;
}

uint64_t parse_timestamp(std::string const& timestamp_str)
{
  std::tm tm = {};
  std::istringstream ss(timestamp_str);
  ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S"); // Extract date and time

  // Extract fractional seconds
  size_t pos = timestamp_str.find('.');
  std::string fractional_seconds_str = timestamp_str.substr(pos + 1, 9);
  uint64_t fractional_seconds = std::stoull(fractional_seconds_str);

  auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
  auto nanoseconds_since_epoch =
    std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch()).count();
  return static_cast<uint64_t>(nanoseconds_since_epoch) + fractional_seconds;
}

bool is_timestamp_ordered(std::vector<std::string> const& file_contents)
{
  constexpr int k_nearest_elements = 5;
  uint64_t previous_timestamp = 0;
  bool first = true;

  for (int i = 0; i < static_cast<int>(file_contents.size()); ++i)
  {
    auto const& line = file_contents[static_cast<size_t>(i)];

    uint64_t current_timestamp = parse_timestamp(line);

    if (!first)
    {
      if (current_timestamp < previous_timestamp)
      {
        std::cout << "Timestamp out of order at index " << i << ". Printing "
                  << k_nearest_elements * 2 << " elements around it:\n";

        int start_index = std::max(0, i - k_nearest_elements);
        int end_index = std::min(static_cast<int>(file_contents.size()) - 1, i + k_nearest_elements);

        for (int j = start_index; j <= end_index; ++j)
        {
          std::cout << file_contents[static_cast<size_t>(j)] << '\n';
        }

        std::cout << "-------------------\n";

        return false;
      }
    }

    previous_timestamp = current_timestamp;
    first = false;
  }

  return true;
}
} // namespace testing
} // namespace quill