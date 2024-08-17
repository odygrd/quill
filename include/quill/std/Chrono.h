/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TriviallyCopyableCodec.h"
#include "quill/core/Attributes.h"
#include "quill/core/Codec.h"
#include "quill/core/DynamicFormatArgStore.h"

#include "quill/bundled/fmt/chrono.h"
#include "quill/bundled/fmt/format.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

QUILL_BEGIN_NAMESPACE

template <template <typename...> class ChronoType, typename TimeSpec, typename DurationType>
struct Codec<ChronoType<TimeSpec, DurationType>,
             std::enable_if_t<std::disjunction_v<std::is_same<ChronoType<TimeSpec, DurationType>, std::chrono::time_point<TimeSpec, DurationType>>,
                                                 std::is_same<ChronoType<TimeSpec, DurationType>, std::chrono::duration<TimeSpec, DurationType>>>>>
  : TriviallyCopyableTypeCodec<ChronoType<TimeSpec, DurationType>>
{
};

QUILL_END_NAMESPACE