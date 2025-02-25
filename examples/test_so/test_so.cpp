#include <iostream>
#include <dlfcn.h>
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include "quill/Backend.h"

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1"));

  LOG_INFO(logger, "start");

  // Open the shared library
  void *handle = dlopen("./libmy_shared_library.so", RTLD_LAZY | RTLD_GLOBAL);
  if (!handle) {
    fprintf(stderr, "Error loading library: %s\n", dlerror());
    return 1;
  }

  // Reset errors
  dlerror();

  // Load the function symbol
  void (*log_message)();
  *(void **)(&log_message) = dlsym(handle, "log_message");

  // Check for errors
  char *error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "Error finding symbol: %s\n", error);
    dlclose(handle);
    return 1;
  }

  // Call the function
  log_message();

  // Close the library
  dlclose(handle);
}