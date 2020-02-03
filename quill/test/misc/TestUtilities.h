#pragma once

#include <fstream>
#include <string>
#include <system_error>
#include <vector>
#include <algorithm>

namespace quill::testing
{
// Convert the given file to a vector
std::vector<std::string> file_contents(std::string const& filename);

// Search a vector for the given string
bool file_contains(std::vector<std::string> const& file_vector, std::string search_string);
} // namespace quill::testing