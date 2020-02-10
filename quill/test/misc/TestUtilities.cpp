#include "TestUtilities.h"

namespace quill
{
namespace testing
{
// Convert the given file to a vector
std::vector<std::string> file_contents(quill::filename_t const& filename)
{
  std::ifstream out_file(filename);

  std::vector<std::string> lines;

  for (std::string current_line; getline(out_file, current_line);)
  {
    lines.push_back(current_line);
  }

  return lines;
}

// Search a vector for the given string
bool file_contains(std::vector<std::string> const& file_vector, std::string search_string)
{
  auto const search =
    std::find_if(file_vector.cbegin(), file_vector.cend(), [&search_string](std::string const& elem) {
      return elem.find(search_string) != std::string::npos;
    });

  return search != file_vector.cend();
}
} // namespace testing
} // namespace quill