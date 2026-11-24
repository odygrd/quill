#pragma once

#include "quill/core/Attributes.h"

#include "quill/Logger.h"
#include <string>

QUILL_EXPORT void setup_quill();
QUILL_EXPORT quill::Logger* get_logger(std::string const& name);

/**
 * Removes the logger of the library, which writes the records that are still queued, and stops
 * the backend. To be called before the library is unloaded. It has C linkage, so that
 * dlsym(handle, "stop_quill") and GetProcAddress(handle, "stop_quill") find it.
 */
extern "C" QUILL_EXPORT void stop_quill();