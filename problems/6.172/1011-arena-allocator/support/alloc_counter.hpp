// alloc_counter.hpp — counts global operator new calls, to detect allocators that use the C++ heap.
#pragma once

#include <cstdint>

namespace alloc_counter {
std::uint64_t operator_new_calls();
}
