#pragma once

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "planted_bugs.hpp"

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

namespace detail_0112 {

inline std::vector<std::string_view> split_lines(std::string_view text) {
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        const std::size_t end = text.find('\n');
        if (end == std::string_view::npos) {
            lines.push_back(text);
            break;
        }
        lines.push_back(text.substr(0, end));
        text.remove_prefix(end + 1);
    }
    return lines;
}

inline bool contains(std::string_view text, std::string_view piece) {
    return text.find(piece) != std::string_view::npos;
}

// "/a/b/file.cpp:12:5" -> "file.cpp:12"; returns "" when no line number follows the file.
inline std::string normalize_location(std::string_view token) {
    const std::size_t slash = token.rfind('/');
    if (slash != std::string_view::npos) {
        token.remove_prefix(slash + 1);
    }
    const std::size_t first_colon = token.find(':');
    if (first_colon == std::string_view::npos || first_colon + 1 >= token.size() ||
        token[first_colon + 1] < '0' || token[first_colon + 1] > '9') {
        return "";
    }
    std::size_t end = first_colon + 1;
    while (end < token.size() && token[end] >= '0' && token[end] <= '9') {
        ++end;
    }
    return std::string(token.substr(0, end));
}

// Frame lines look like: "    #3 0x0001 in some_function(int) dir/file.cpp:12"
// Frames inside the runtime end with "(libclang_rt...)" and have no file:line token.
inline bool read_frame(std::string_view line, std::string& function, std::string& location) {
    const std::size_t in = line.find(" in ");
    if (in == std::string_view::npos || !contains(line.substr(0, in), "#")) {
        return false;
    }
    std::string_view rest = line.substr(in + 4);
    while (!rest.empty() && rest.back() == ' ') {
        rest.remove_suffix(1);
    }
    const std::size_t last_space = rest.rfind(' ');
    if (last_space == std::string_view::npos) {
        return false;
    }
    const std::string where = normalize_location(rest.substr(last_space + 1));
    if (where.empty()) {
        return false;
    }
    function = std::string(rest.substr(0, last_space));
    location = where;
    return true;
}

inline std::size_t parse_size(std::string_view text) {
    std::size_t value = 0;
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

inline BugClass classify_asan(std::string_view message) {
    if (contains(message, "heap-use-after-free")) {
        return BugClass::HeapUseAfterFree;
    }
    if (contains(message, "heap-buffer-overflow")) {
        return BugClass::HeapBufferOverflow;
    }
    if (contains(message, "stack-buffer-overflow")) {
        return BugClass::StackBufferOverflow;
    }
    if (contains(message, "double-free")) {
        return BugClass::DoubleFree;
    }
    if (contains(message, "SEGV on unknown address")) {
        const std::size_t hex = message.find("0x");
        if (hex != std::string_view::npos) {
            std::uint64_t address = 0;
            const char* const begin = message.data() + hex + 2;
            const char* const end = message.data() + message.size();
            const auto [next, error] = std::from_chars(begin, end, address, 16);
            if (error == std::errc{} && next != begin && address < 4096) {
                return BugClass::NullDereference;
            }
        }
    }
    return BugClass::Unknown;
}

inline BugClass classify_ubsan(std::string_view message) {
    if (contains(message, "signed integer overflow")) {
        return BugClass::SignedIntegerOverflow;
    }
    if (contains(message, "shift exponent") || contains(message, "left shift of") ||
        contains(message, "right shift of")) {
        return BugClass::ShiftOutOfRange;
    }
    if (contains(message, "misaligned address")) {
        return BugClass::MisalignedAccess;
    }
    if (contains(message, "null pointer")) {
        return BugClass::NullDereference;
    }
    if (contains(message, "out of bounds")) {
        return BugClass::OutOfBoundsIndex;
    }
    return BugClass::Unknown;
}

}  // namespace detail_0112

inline SanitizerReport parse_report(std::string_view log) {
    using namespace detail_0112;
    constexpr std::string_view kAsanMarker = "ERROR: AddressSanitizer:";
    constexpr std::string_view kUbsanMarker = ": runtime error: ";

    SanitizerReport report;
    const std::vector<std::string_view> lines = split_lines(log);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string_view line = lines[i];
        if (const std::size_t at = line.find(kAsanMarker); at != std::string_view::npos) {
            report.tool = "AddressSanitizer";
            report.bug = classify_asan(line.substr(at + kAsanMarker.size()));
            for (std::size_t k = i + 1; k < lines.size(); ++k) {
                const std::string_view detail = lines[k];
                std::string function;
                std::string location;
                if (report.access.empty()) {
                    for (const std::string_view op : {"READ", "WRITE"}) {
                        const std::string prefix = std::string(op) + " of size ";
                        if (const std::size_t found = detail.find(prefix);
                            found != std::string_view::npos) {
                            report.access = std::string(op);
                            report.access_size = parse_size(detail.substr(found + prefix.size()));
                        }
                    }
                }
                if (report.top_frame.empty() && read_frame(detail, function, location)) {
                    report.top_frame = std::move(function);
                    report.location = std::move(location);
                }
                if (detail.find("ERROR: AddressSanitizer:") != std::string_view::npos) {
                    break;  // a second report starts here
                }
            }
            return report;
        }
        if (const std::size_t at = line.find(kUbsanMarker); at != std::string_view::npos) {
            report.tool = "UndefinedBehaviorSanitizer";
            report.bug = classify_ubsan(line.substr(at + kUbsanMarker.size()));
            report.location = normalize_location(line.substr(0, at));
            return report;
        }
    }
    return report;
}

struct CaptureResult {
    int exit_code = 0;
    int signal = 0;
    std::string output;
};

inline CaptureResult capture_stderr(std::function<void()> fn) {
    int pipe_ends[2];
    if (::pipe(pipe_ends) != 0) {
        throw std::runtime_error("pipe failed");
    }
    const ::pid_t child = ::fork();
    if (child < 0) {
        throw std::runtime_error("fork failed");
    }
    if (child == 0) {
        ::close(pipe_ends[0]);
        ::dup2(pipe_ends[1], STDERR_FILENO);
        ::close(pipe_ends[1]);
        try {
            fn();
        } catch (...) {
            ::_exit(1);
        }
        // _exit, not exit: the child must not flush buffers or run destructors it inherited.
        ::_exit(0);
    }

    ::close(pipe_ends[1]);
    CaptureResult result;
    // Drain before waiting: a child that fills the pipe would otherwise block forever.
    char buffer[4096];
    for (;;) {
        const ::ssize_t got = ::read(pipe_ends[0], buffer, sizeof(buffer));
        if (got > 0) {
            result.output.append(buffer, static_cast<std::size_t>(got));
        } else if (got == 0 || (got < 0 && errno != EINTR)) {
            break;
        }
    }
    ::close(pipe_ends[0]);

    int status = 0;
    while (::waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            throw std::runtime_error("waitpid failed");
        }
    }
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exit_code = -1;
        result.signal = WIFSIGNALED(status) ? WTERMSIG(status) : 0;
    }
    return result;
}

inline SanitizerReport triage(std::function<void()> fn) {
    return parse_report(capture_stderr(std::move(fn)).output);
}

// Stage 4
inline int first_after_push(std::vector<int>& values, int extra) {
    const int first = values.front();  // copy the value, not a pointer into the old block
    values.push_back(extra);
    return first + extra;
}

inline std::unique_ptr<int[]> clone_prefix(const int* src, std::size_t n) {
    auto copy = std::make_unique<int[]>(n);
    std::copy_n(src, n, copy.get());
    return copy;
}

inline std::size_t copy_truncated(const char* text, std::span<char> out) {
    const std::size_t length = std::strlen(text);
    if (!out.empty()) {
        const std::size_t copied = std::min(length, out.size() - 1);
        std::memcpy(out.data(), text, copied);
        out[copied] = '\0';
    }
    return length;
}

class OwnedBuffer {
public:
    explicit OwnedBuffer(std::size_t n) : size_(n), data_(std::make_unique<int[]>(n)) {}

    OwnedBuffer(const OwnedBuffer& other)
        : size_(other.size_), data_(std::make_unique<int[]>(other.size_)) {
        std::copy_n(other.data_.get(), size_, data_.get());
    }

    OwnedBuffer& operator=(const OwnedBuffer& other) {
        if (this != &other) {
            auto copy = std::make_unique<int[]>(other.size_);
            std::copy_n(other.data_.get(), other.size_, copy.get());
            data_ = std::move(copy);
            size_ = other.size_;
        }
        return *this;
    }

    ~OwnedBuffer() = default;

    int* data() { return data_.get(); }
    const int* data() const { return data_.get(); }
    std::size_t size() const { return size_; }

private:
    std::size_t size_;
    std::unique_ptr<int[]> data_;
};

inline long long sum_wide(const std::vector<int>& values) {
    long long total = 0;
    for (const int value : values) {
        total += value;
    }
    return total;
}

inline std::uint32_t low_bits_mask_safe(int count) {
    if (count <= 0) {
        return 0;
    }
    if (count >= 32) {
        return 0xFFFFFFFFU;
    }
    return (std::uint32_t{1} << count) - 1;
}

inline std::uint32_t read_u32_safe(const std::uint8_t* buffer, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, buffer + offset, sizeof(value));
    return value;
}

inline std::optional<int> second_value_safe(const planted::ListNode* head) {
    if (head == nullptr || head->next == nullptr) {
        return std::nullopt;
    }
    return head->next->value;
}

inline void record_bucket_safe(std::span<int> counts, int bucket) {
    if (bucket < 0 || static_cast<std::size_t>(bucket) >= counts.size()) {
        throw std::out_of_range("bucket out of range");
    }
    ++counts[static_cast<std::size_t>(bucket)];
}

// Stage 5
struct CrashGroup {
    SanitizerReport report;
    std::size_t count = 0;
};

inline std::vector<CrashGroup> group_reports(std::span<const std::string> logs) {
    std::map<std::tuple<BugClass, std::string, std::string>, CrashGroup> groups;
    std::vector<std::tuple<BugClass, std::string, std::string>> order;
    for (const std::string& log : logs) {
        SanitizerReport report = parse_report(log);
        if (report.bug == BugClass::None) {
            continue;
        }
        auto key = std::make_tuple(report.bug, report.top_frame, report.location);
        auto [slot, inserted] = groups.try_emplace(key);
        if (inserted) {
            slot->second.report = std::move(report);
        }
        ++slot->second.count;
    }
    std::vector<CrashGroup> result;
    result.reserve(groups.size());
    for (auto& [key, group] : groups) {
        result.push_back(std::move(group));
    }
    std::stable_sort(result.begin(), result.end(), [](const CrashGroup& a, const CrashGroup& b) {
        if (a.count != b.count) {
            return a.count > b.count;
        }
        return a.report.location < b.report.location;
    });
    return result;
}
