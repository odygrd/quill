/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/sinks/JsonSink.h"
#include "quill/sinks/RotatingSink.h"

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

using RotatingJsonFileSink = RotatingSink<JsonFileSink>;

QUILL_END_EXPORT

QUILL_END_NAMESPACE