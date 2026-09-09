#include "quill/bundled/fmt/format.h"

int main()
{
  return fmtquill::format("Bundled formatter: {}", 42) == "Bundled formatter: 42" ? 0 : 1;
}
