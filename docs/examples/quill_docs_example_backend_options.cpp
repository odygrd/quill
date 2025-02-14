#include "quill/Backend.h"

int main()
{
  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = 5;
  quill::Backend::start(backend_options);
}
