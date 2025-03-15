#pragma once

#include "quill/core/Attributes.h"

#include "quill/Logger.h"
#include <string>

QUILL_EXPORT void setup_quill();
QUILL_EXPORT quill::Logger* get_logger(std::string const& name);