#pragma once

#include "quill/core/Attributes.h"

#include "quill/Logger.h"
#include <string>

void setup_quill();
quill::Logger* get_logger(std::string const& name);