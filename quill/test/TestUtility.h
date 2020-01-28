#pragma once

#include <fstream>
#include <string>
#include <system_error>

namespace quill::testing
{
inline std::string file_contents(std::string const& filename)
{
  std::ifstream ifs(filename);
  if (!ifs)
  {
    throw std::system_error((errno), std::generic_category());
  }
  return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
}
} // namespace quill::testing