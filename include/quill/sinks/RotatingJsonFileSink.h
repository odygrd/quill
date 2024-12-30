/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/sinks/RotatingSink.h"
#include "quill/sinks/JsonSink.h"

QUILL_BEGIN_NAMESPACE

using RotatingJsonFileSink = RotatingSink<JsonFileSink>;

QUILL_END_NAMESPACE