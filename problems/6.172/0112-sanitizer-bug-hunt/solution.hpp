#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "planted_bugs.hpp"
#include "practice.hpp"

enum class BugClass {
    None,
    Unknown,
    HeapUseAfterFree,
    HeapBufferOverflow,
    StackBufferOverflow,
    DoubleFree,
    SignedIntegerOverflow,
    ShiftOutOfRange,
    MisalignedAccess,
    NullDereference,
    OutOfBoundsIndex,
};

struct SanitizerReport {
    BugClass bug = BugClass::None;
    std::string tool;
    std::string access;
    std::size_t access_size = 0;
    std::string top_frame;
    std::string location;
};

// Stage 1
inline SanitizerReport parse_report(std::string_view log) { PRACTICE_TODO(); }

// Stage 2
struct CaptureResult {
    int exit_code = 0;
    int signal = 0;
    std::string output;
};

inline CaptureResult capture_stderr(std::function<void()> fn) { PRACTICE_TODO(); }

// Stage 3
inline SanitizerReport triage(std::function<void()> fn) { PRACTICE_TODO(); }

// Stage 4: corrected versions of the planted bugs.
inline int first_after_push(std::vector<int>& values, int extra) { PRACTICE_TODO(); }
inline std::unique_ptr<int[]> clone_prefix(const int* src, std::size_t n) { PRACTICE_TODO(); }
inline std::size_t copy_truncated(const char* text, std::span<char> out) { PRACTICE_TODO(); }

class OwnedBuffer {
public:
    explicit OwnedBuffer(std::size_t n) { PRACTICE_TODO(); }
    ~OwnedBuffer() {}
    OwnedBuffer(const OwnedBuffer& other) { PRACTICE_TODO(); }
    OwnedBuffer& operator=(const OwnedBuffer& other) { PRACTICE_TODO(); }

    int* data() { PRACTICE_TODO(); }
    const int* data() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};

inline long long sum_wide(const std::vector<int>& values) { PRACTICE_TODO(); }
inline std::uint32_t low_bits_mask_safe(int count) { PRACTICE_TODO(); }
inline std::uint32_t read_u32_safe(const std::uint8_t* buffer, std::size_t offset) {
    PRACTICE_TODO();
}
inline std::optional<int> second_value_safe(const planted::ListNode* head) { PRACTICE_TODO(); }
inline void record_bucket_safe(std::span<int> counts, int bucket) { PRACTICE_TODO(); }

// Stage 5
struct CrashGroup {
    SanitizerReport report;
    std::size_t count = 0;
};

inline std::vector<CrashGroup> group_reports(std::span<const std::string> logs) { PRACTICE_TODO(); }
