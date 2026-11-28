#include "quill/LogMacros.h"
#include "quill/SimpleSetup.h"
#include "quill/std/Vector.h"

#include <vector>

int main()
{
  auto* logger = quill::simple_logger();
  std::vector<int> ports{8080, 8443};
  LOG_INFO(logger, "Listening on ports {}", ports);
}
