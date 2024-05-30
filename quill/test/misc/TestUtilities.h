#pragma once

#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"

#include "DocTestExtensions.h"

#include <cstring>
#include <string>
#include <vector>

namespace quill
{
namespace testing
{
// Convert the given file to a vector
std::vector<std::string> file_contents(fs::path const& filename);
std::vector<std::wstring> wfile_contents(fs::path const& filename);

// Search a vector for the given string
bool file_contains(std::vector<std::string> const& file_vector, std::string const& search_string);
void create_file(fs::path const& filename, std::string const& text = std::string{});
void remove_file(fs::path const& filename);

std::vector<std::string> gen_random_strings(size_t n, int min_len, int max_len);

uint64_t parse_timestamp(std::string const& timestamp_str);
bool is_timestamp_ordered(std::vector<std::string> const& file_contents);
} // namespace testing
} // namespace quill
