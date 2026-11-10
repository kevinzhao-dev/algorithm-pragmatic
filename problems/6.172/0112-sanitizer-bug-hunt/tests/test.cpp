#include <signal.h>
#include <unistd.h>

#include <climits>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define PRACTICE_ASAN_BUILD 1
#endif
#endif
#ifndef PRACTICE_ASAN_BUILD
#define PRACTICE_ASAN_BUILD 0
#endif

namespace {

constexpr bool kAsanBuild = PRACTICE_ASAN_BUILD == 1;

bool has(const std::string& text, const char* piece) {
    return text.find(piece) != std::string::npos;
}

const char* const kUseAfterFree = R"(unit test output before the report
=================================================================
==4242==ERROR: AddressSanitizer: heap-use-after-free on address 0x6020000000d4 at pc 0x00010042c9d8 bp 0x00016f9d1240 sp 0x00016f9d1238
READ of size 4 at 0x6020000000d4 thread T0
    #0 0x00010042c9d4 in RingBuffer::peek() const /home/dev/project/src/ring_buffer.cpp:88:14
    #1 0x00010042d1a8 in worker_loop(int) /home/dev/project/src/worker.cpp:41:9
    #2 0x00018b1404e0 in start+0x1b4c (dyld:arm64e+0x204e0)

0x6020000000d4 is located 4 bytes inside of 16-byte region [0x6020000000d0,0x6020000000e0)
freed by thread T0 here:
    #0 0x0001009dc644 in operator delete[](void*)+0x74 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x50644)
    #1 0x00010042c964 in RingBuffer::grow() /home/dev/project/src/ring_buffer.cpp:30:3
)";

const char* const kHeapOverflow = R"(
==77==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000014 at pc 0x000100ab0000 bp 0x00016f000000 sp 0x00016f000008
WRITE of size 8 at 0x602000000014 thread T0
    #0 0x000100ab0000 in __asan_memcpy+0x394 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x12345)
    #1 0x000100ab0010 in append_row(Table&, Row const&) table.cc:210
    #2 0x000100ab0020 in main main.cc:7
)";

const char* const kStackOverflow =
    "==5==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x00016f000020 at pc 0x1 bp 0x2 "
    "sp 0x3\nWRITE of size 1 at 0x00016f000020 thread T0\n    #0 0x000100001000 in format_id(char*, "
    "int) /tmp/x/id.cpp:15:7\n";

const char* const kDoubleFree = R"(
==9==ERROR: AddressSanitizer: attempting double-free on 0x602000000010 in thread T0:
    #0 0x0001009dc644 in operator delete[](void*)+0x74 (libclang_rt.asan_osx_dynamic.dylib:arm64e+0x50644)
    #1 0x000100002000 in Holder::~Holder() /src/holder.h:12:9
    #2 0x000100002100 in main /src/main.cpp:40:1
)";

const char* const kSegvNull = R"(
==3==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000000 (pc 0x000100001000 bp 0x00016f000000 sp 0x00016f000008 T0)
==3==The signal is caused by a READ memory access.
==3==Hint: address points to the zero page.
    #0 0x000100001000 in load_config(Settings*) /repo/load.cpp:99:30
)";

const char* const kSegvWild = R"(
==3==ERROR: AddressSanitizer: SEGV on unknown address 0x7f00deadbeef (pc 0x1 bp 0x2 sp 0x3 T0)
    #0 0x000100001000 in scan_all(Node*) /repo/scan.cpp:55
)";

const char* const kSignedOverflow =
    "src/metrics.cpp:12:30: runtime error: signed integer overflow: 2147483647 + 1 cannot be "
    "represented in type 'int'\nSUMMARY: UndefinedBehaviorSanitizer: undefined-behavior "
    "src/metrics.cpp:12:30\n";

const char* const kShift =
    "/a/b/encode.cpp:88:9: runtime error: shift exponent 32 is too large for 32-bit type 'unsigned "
    "int'\n";

const char* const kMisaligned =
    "load.cpp:4:10: runtime error: load of misaligned address 0x00016f9d11e1 for type 'int', which "
    "requires 4 byte alignment\n";

const char* const kNull =
    "graph.cpp:71:23: runtime error: member access within null pointer of type 'Node'\n";

const char* const kBounds =
    "hist.cpp:5:18: runtime error: index 9 out of bounds for type 'int[8]'\n";

}  // namespace

TEST(1, "use-after-free report is fully parsed") {
    const SanitizerReport r = parse_report(kUseAfterFree);
    CHECK_EQ(r.bug, BugClass::HeapUseAfterFree);
    CHECK_EQ(r.tool, "AddressSanitizer");
    CHECK_EQ(r.access, "READ");
    CHECK_EQ(r.access_size, 4);
    CHECK(has(r.top_frame, "RingBuffer::peek"));
    CHECK_EQ(r.location, "ring_buffer.cpp:88");
}

TEST(1, "runtime frames without source locations are skipped") {
    const SanitizerReport r = parse_report(kHeapOverflow);
    CHECK_EQ(r.bug, BugClass::HeapBufferOverflow);
    CHECK_EQ(r.access, "WRITE");
    CHECK_EQ(r.access_size, 8);
    CHECK(has(r.top_frame, "append_row"));
    CHECK_EQ(r.location, "table.cc:210");
}

TEST(1, "stack overflow and double-free are classified") {
    const SanitizerReport stack = parse_report(kStackOverflow);
    CHECK_EQ(stack.bug, BugClass::StackBufferOverflow);
    CHECK_EQ(stack.access_size, 1);
    CHECK_EQ(stack.location, "id.cpp:15");

    const SanitizerReport twice = parse_report(kDoubleFree);
    CHECK_EQ(twice.bug, BugClass::DoubleFree);
    CHECK_EQ(twice.access, "");
    CHECK_EQ(twice.access_size, 0);
    CHECK(has(twice.top_frame, "Holder::~Holder"));
    CHECK_EQ(twice.location, "holder.h:12");
}

TEST(1, "a SEGV is a null dereference only near address zero") {
    CHECK_EQ(parse_report(kSegvNull).bug, BugClass::NullDereference);
    CHECK_EQ(parse_report(kSegvWild).bug, BugClass::Unknown);
}

TEST(1, "UBSan reports fill location and leave access fields empty") {
    const SanitizerReport r = parse_report(kSignedOverflow);
    CHECK_EQ(r.bug, BugClass::SignedIntegerOverflow);
    CHECK_EQ(r.tool, "UndefinedBehaviorSanitizer");
    CHECK_EQ(r.access, "");
    CHECK_EQ(r.top_frame, "");
    CHECK_EQ(r.location, "metrics.cpp:12");
}

TEST(1, "each UBSan category is recognised") {
    CHECK_EQ(parse_report(kShift).bug, BugClass::ShiftOutOfRange);
    CHECK_EQ(parse_report(kMisaligned).bug, BugClass::MisalignedAccess);
    CHECK_EQ(parse_report(kNull).bug, BugClass::NullDereference);
    CHECK_EQ(parse_report(kBounds).bug, BugClass::OutOfBoundsIndex);
    CHECK_EQ(parse_report("x.cpp:1:2: runtime error: something unusual happened\n").bug,
             BugClass::Unknown);
}

TEST(1, "the first report wins and ordinary text is no report") {
    const std::string log = std::string(kSignedOverflow) + kUseAfterFree;
    CHECK_EQ(parse_report(log).bug, BugClass::SignedIntegerOverflow);
    const SanitizerReport none = parse_report("all 17 tests passed\n");
    CHECK_EQ(none.bug, BugClass::None);
    CHECK_EQ(none.tool, "");
    CHECK_EQ(parse_report("").bug, BugClass::None);
}

TEST(2, "captures what the child writes to stderr") {
    const CaptureResult result = capture_stderr([] { std::fputs("from child\n", stderr); });
    CHECK_EQ(result.output, "from child\n");
    CHECK_EQ(result.exit_code, 0);
    CHECK_EQ(result.signal, 0);
}

TEST(2, "reports the child's exit code") {
    CHECK_EQ(capture_stderr([] { ::_exit(3); }).exit_code, 3);
    CHECK_EQ(capture_stderr([] {}).exit_code, 0);
}

TEST(2, "reports death by signal") {
    const CaptureResult result = capture_stderr([] { ::raise(SIGKILL); });
    CHECK_EQ(result.signal, SIGKILL);
    CHECK_EQ(result.exit_code, -1);
}

TEST(2, "an exception in the child stays in the child") {
    const CaptureResult result = capture_stderr([] { throw std::runtime_error("boom"); });
    CHECK_EQ(result.exit_code, 1);
    CHECK_EQ(result.signal, 0);
    // Reaching here means the child did not unwind back through the test runner.
    CHECK(true);
}

TEST(2, "long output neither deadlocks nor truncates") {
    const std::size_t total = 2 * 1024 * 1024;
    const CaptureResult result = capture_stderr([total] {
        const std::string block(64 * 1024, 'x');
        std::size_t written = 0;
        while (written < total) {
            ::write(STDERR_FILENO, block.data(), block.size());
            written += block.size();
        }
    });
    CHECK_EQ(result.output.size(), total);
    CHECK_EQ(result.exit_code, 0);
}

TEST(3, "a clean run yields no report") {
    if (!kAsanBuild) {
        return;
    }
    CHECK_EQ(triage([] {}).bug, BugClass::None);
}

TEST(3, "use after realloc is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] {
        std::vector<int> values = {1, 2, 3};
        std::printf("%d\n", planted::use_after_realloc(values, 9));
    });
    INFO("top frame: ", r.top_frame);
    CHECK_EQ(r.bug, BugClass::HeapUseAfterFree);
    CHECK_EQ(r.access, "READ");
    CHECK_EQ(r.access_size, 4);
    CHECK(has(r.top_frame, "use_after_realloc"));
    CHECK(has(r.location, "planted_bugs.hpp"));
}

TEST(3, "off-by-one write is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] {
        const int source[] = {1, 2, 3, 4, 5};
        delete[] planted::clone_off_by_one(source, 4);
    });
    INFO("top frame: ", r.top_frame);
    CHECK_EQ(r.bug, BugClass::HeapBufferOverflow);
    CHECK_EQ(r.access, "WRITE");
    CHECK(has(r.top_frame, "clone_off_by_one"));
}

TEST(3, "stack smash is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r =
        triage([] { std::printf("%d\n", planted::copy_into_stack("0123456789abcdef")); });
    INFO("top frame: ", r.top_frame);
    CHECK_EQ(r.bug, BugClass::StackBufferOverflow);
    CHECK_EQ(r.access, "WRITE");
    CHECK(has(r.top_frame, "copy_into_stack"));
}

TEST(3, "double free is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] { std::printf("%d\n", planted::double_release()); });
    INFO("top frame: ", r.top_frame);
    CHECK_EQ(r.bug, BugClass::DoubleFree);
    CHECK(has(r.top_frame, "ShallowBuffer") || has(r.top_frame, "double_release"));
}

TEST(3, "signed overflow is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r =
        triage([] { std::printf("%d\n", planted::sum_values({INT_MAX, 1})); });
    CHECK_EQ(r.bug, BugClass::SignedIntegerOverflow);
    CHECK_EQ(r.tool, "UndefinedBehaviorSanitizer");
    CHECK(has(r.location, "planted_bugs.hpp"));
}

TEST(3, "oversized shift is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] { std::printf("%u\n", planted::low_bits_mask(32)); });
    CHECK_EQ(r.bug, BugClass::ShiftOutOfRange);
}

TEST(3, "misaligned load is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] {
        std::uint8_t bytes[16] = {1, 2, 3, 4, 5, 6, 7, 8};
        std::printf("%u\n", planted::read_u32(bytes, 1));
    });
    CHECK_EQ(r.bug, BugClass::MisalignedAccess);
}

TEST(3, "null pointer use is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] {
        const planted::ListNode only{7, nullptr};
        std::printf("%d\n", planted::second_value(&only));
    });
    CHECK_EQ(r.bug, BugClass::NullDereference);
}

TEST(3, "out-of-bounds index is caught live") {
    if (!kAsanBuild) {
        return;
    }
    const SanitizerReport r = triage([] { std::printf("%d\n", planted::record_bucket(4)); });
    CHECK_EQ(r.bug, BugClass::OutOfBoundsIndex);
}

TEST(4, "first_after_push survives reallocation") {
    std::vector<int> values = {1, 2, 3};
    CHECK_EQ(first_after_push(values, 9), 10);
    CHECK_EQ(values, std::vector<int>{1, 2, 3, 9});
}

TEST(4, "clone_prefix copies exactly n elements") {
    const int source[] = {4, 5, 6, 7};
    const auto copy = clone_prefix(source, 3);
    CHECK_EQ(copy[0], 4);
    CHECK_EQ(copy[2], 6);
    CHECK(clone_prefix(source, 0) != nullptr);
}

TEST(4, "copy_truncated follows strlcpy semantics") {
    char buffer[8];
    CHECK_EQ(copy_truncated("short", buffer), 5);
    CHECK_EQ(std::string(buffer), "short");
    CHECK_EQ(copy_truncated("exactly7", buffer), 8);
    CHECK_EQ(std::string(buffer), "exactly");
    CHECK_EQ(copy_truncated("x", std::span<char>()), 1);
    char single[1];
    CHECK_EQ(copy_truncated("abc", single), 3);
    CHECK_EQ(single[0], '\0');
    CHECK_EQ(copy_truncated("", buffer), 0);
    CHECK_EQ(buffer[0], '\0');
}

TEST(4, "OwnedBuffer copies deeply and tolerates self assignment") {
    OwnedBuffer a(4);
    CHECK_EQ(a.size(), 4);
    a.data()[0] = 11;
    OwnedBuffer b = a;
    b.data()[0] = 22;
    CHECK_EQ(a.data()[0], 11);
    CHECK_EQ(b.data()[0], 22);

    OwnedBuffer c(1);
    c = a;
    c.data()[3] = 7;
    CHECK_EQ(a.data()[3], 0);
    CHECK_EQ(c.size(), 4);
    c = static_cast<OwnedBuffer&>(c);
    CHECK_EQ(c.data()[3], 7);
}

TEST(4, "sum_wide does not overflow") {
    CHECK_EQ(sum_wide({INT_MAX, 1}), 2147483648LL);
    CHECK_EQ(sum_wide({INT_MIN, -1}), -2147483649LL);
    std::vector<int> many(1000, INT_MAX);
    CHECK_EQ(sum_wide(many), 1000LL * INT_MAX);
    CHECK_EQ(sum_wide({}), 0);
}

TEST(4, "low_bits_mask_safe covers every count including the extremes") {
    CHECK_EQ(low_bits_mask_safe(0), 0U);
    CHECK_EQ(low_bits_mask_safe(1), 1U);
    CHECK_EQ(low_bits_mask_safe(31), 0x7FFF'FFFFU);
    CHECK_EQ(low_bits_mask_safe(32), 0xFFFF'FFFFU);
}

TEST(4, "read_u32_safe handles unaligned offsets") {
    std::uint8_t bytes[12] = {0xAA, 0x11, 0x22, 0x33, 0x44, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0};
    std::uint32_t expected = 0;
    std::memcpy(&expected, bytes + 1, 4);
    CHECK_EQ(read_u32_safe(bytes, 1), expected);
    std::memcpy(&expected, bytes + 5, 4);
    CHECK_EQ(read_u32_safe(bytes, 5), expected);
}

TEST(4, "second_value_safe handles short lists") {
    planted::ListNode second{9, nullptr};
    planted::ListNode first{1, &second};
    planted::ListNode alone{5, nullptr};
    CHECK_EQ(second_value_safe(&first), std::optional<int>(9));
    CHECK_EQ(second_value_safe(&alone), std::optional<int>());
    CHECK_EQ(second_value_safe(nullptr), std::optional<int>());
}

TEST(4, "record_bucket_safe rejects out-of-range buckets") {
    std::vector<int> counts(4, 0);
    record_bucket_safe(counts, 3);
    CHECK_EQ(counts[3], 1);
    CHECK_THROWS_AS(record_bucket_safe(counts, 4), std::out_of_range);
    CHECK_THROWS_AS(record_bucket_safe(counts, -1), std::out_of_range);
    CHECK_EQ(counts, std::vector<int>{0, 0, 0, 1});
}

TEST(5, "identical crashes collapse into one group") {
    const std::vector<std::string> logs = {kUseAfterFree, kUseAfterFree, kUseAfterFree};
    const std::vector<CrashGroup> groups = group_reports(logs);
    REQUIRE_EQ(groups.size(), 1);
    CHECK_EQ(groups[0].count, 3);
    CHECK_EQ(groups[0].report.bug, BugClass::HeapUseAfterFree);
}

TEST(5, "groups are ordered by count, then location") {
    const std::vector<std::string> logs = {
        kSignedOverflow, kHeapOverflow, kSignedOverflow, kShift, kBounds, kBounds, kSignedOverflow,
    };
    const std::vector<CrashGroup> groups = group_reports(logs);
    REQUIRE_EQ(groups.size(), 4);
    CHECK_EQ(groups[0].report.bug, BugClass::SignedIntegerOverflow);
    CHECK_EQ(groups[0].count, 3);
    CHECK_EQ(groups[1].report.bug, BugClass::OutOfBoundsIndex);
    CHECK_EQ(groups[1].count, 2);
    // count 1: "encode.cpp:88" (shift) sorts before "table.cc:210" (heap overflow)
    CHECK_EQ(groups[2].report.bug, BugClass::ShiftOutOfRange);
    CHECK_EQ(groups[3].report.bug, BugClass::HeapBufferOverflow);
}

TEST(5, "same bug at different lines stays separate") {
    const std::string first = "/x/a.cpp:5:1: runtime error: signed integer overflow: 1 + 1\n";
    const std::string second = "/x/a.cpp:9:1: runtime error: signed integer overflow: 2 + 2\n";
    const std::vector<std::string> logs = {first, second, first};
    const std::vector<CrashGroup> groups = group_reports(logs);
    REQUIRE_EQ(groups.size(), 2);
    CHECK_EQ(groups[0].report.location, "a.cpp:5");
    CHECK_EQ(groups[0].count, 2);
}

TEST(5, "logs without a report are ignored") {
    const std::vector<std::string> logs = {"just some output\n", kShift, ""};
    const std::vector<CrashGroup> groups = group_reports(logs);
    REQUIRE_EQ(groups.size(), 1);
    CHECK_EQ(groups[0].report.bug, BugClass::ShiftOutOfRange);
}

TEST(5, "no logs means no groups") {
    CHECK(group_reports(std::span<const std::string>()).empty());
}
