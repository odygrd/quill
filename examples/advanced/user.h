#pragma once

#include <array>
#include <cstdint>
#include <string>

/**
 * User defined type
 */
struct User
{
  std::string name;
  std::string surname;
  uint32_t age;
  std::array<std::string, 3> favorite_colors;
};