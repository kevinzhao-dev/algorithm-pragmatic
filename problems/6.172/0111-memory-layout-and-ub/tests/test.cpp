#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

struct Mixed {
    char a;
    int b;
    char c;
    double d;
    short e;
};

struct Trailing {
    long long x;
    char y;
};

struct Reordered {
    double d;
    int b;
    short e;
    char a;
    char c;
};

template <typename T>
FieldSpec spec() {
    return {sizeof(T), alignof(T)};
}

std::size_t brute_min_size(std::vector<FieldSpec> fields) {
    std::vector<std::size_t> order(fields.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::size_t best = std::numeric_limits<std::size_t>::max();
    do {
        std::vector<FieldSpec> permuted;
        for (std::size_t index : order) {
            permuted.push_back(fields[index]);
        }
        best = std::min(best, compute_layout(permuted).size);
    } while (std::next_permutation(order.begin(), order.end()));
    return best;
}

struct NoDefault {
    explicit NoDefault(int v) : value(v) {}
    int value;
};

struct alignas(64) OverAligned {
    int value = 0;
};

struct Throwing {
    static inline int live = 0;
    static inline int throw_on_copy_number = -1;
    static inline int copies = 0;

    Throwing() { ++live; }
    Throwing(const Throwing& other) : value(other.value) {
        if (copies++ == throw_on_copy_number) {
            throw std::runtime_error("copy failed");
        }
        ++live;
    }
    Throwing& operator=(const Throwing&) = default;
    ~Throwing() { --live; }

    int value = 0;
};

}  // namespace

TEST(1, "no fields gives a one-byte struct") {
    const std::vector<FieldSpec> fields;
    const StructLayout layout = compute_layout(fields);
    CHECK(layout.offsets.empty());
    CHECK_EQ(layout.size, 1);
    CHECK_EQ(layout.align, 1);
}

TEST(1, "offsets follow alignment and the end is rounded up") {
    const std::vector<FieldSpec> fields = {{1, 1}, {4, 4}, {1, 1}};
    const StructLayout layout = compute_layout(fields);
    CHECK_EQ(layout.offsets, std::vector<std::size_t>{0, 4, 8});
    CHECK_EQ(layout.size, 12);
    CHECK_EQ(layout.align, 4);
}

TEST(1, "agrees with the real struct layout on this ABI") {
    const std::vector<FieldSpec> mixed = {spec<char>(), spec<int>(), spec<char>(), spec<double>(),
                                          spec<short>()};
    const StructLayout layout = compute_layout(mixed);
    CHECK_EQ(layout.offsets[0], offsetof(Mixed, a));
    CHECK_EQ(layout.offsets[1], offsetof(Mixed, b));
    CHECK_EQ(layout.offsets[2], offsetof(Mixed, c));
    CHECK_EQ(layout.offsets[3], offsetof(Mixed, d));
    CHECK_EQ(layout.offsets[4], offsetof(Mixed, e));
    CHECK_EQ(layout.size, sizeof(Mixed));
    CHECK_EQ(layout.align, alignof(Mixed));

    const std::vector<FieldSpec> trailing = {spec<long long>(), spec<char>()};
    const StructLayout tail = compute_layout(trailing);
    CHECK_EQ(tail.size, sizeof(Trailing));
    CHECK_EQ(tail.align, alignof(Trailing));
}

TEST(1, "a field that is not a multiple of its alignment is rejected") {
    const std::vector<FieldSpec> fields = {{4, 8}};
    CHECK_THROWS_AS(compute_layout(fields), std::invalid_argument);
    CHECK_THROWS_AS(padding_bytes(fields), std::invalid_argument);
}

TEST(1, "padding_bytes reports interior and tail padding") {
    CHECK_EQ(padding_bytes(std::vector<FieldSpec>{{1, 1}, {4, 4}, {1, 1}}), 6);
    CHECK_EQ(padding_bytes(std::vector<FieldSpec>{{4, 4}, {4, 4}}), 0);
}

TEST(1, "packed_order reproduces the hand-sorted struct") {
    const std::vector<FieldSpec> fields = {spec<char>(), spec<int>(), spec<char>(), spec<double>(),
                                           spec<short>()};
    const std::vector<std::size_t> order = packed_order(fields);
    std::vector<FieldSpec> permuted;
    for (const std::size_t index : order) {
        permuted.push_back(fields[index]);
    }
    CHECK_EQ(compute_layout(permuted).size, sizeof(Reordered));
}

TEST(1, "packed_order is a permutation and matches brute force") {
    Rng rng;
    const std::array<std::size_t, 5> aligns = {1, 2, 4, 8, 16};
    for (int round = 0; round < 300; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 6));
        std::vector<FieldSpec> fields(n);
        for (FieldSpec& field : fields) {
            const std::size_t align = aligns[static_cast<std::size_t>(rng.uniform(0, 4))];
            field = {align * static_cast<std::size_t>(rng.uniform(1, 5)), align};
        }
        std::vector<std::size_t> order = packed_order(fields);
        std::vector<std::size_t> expected(n);
        std::iota(expected.begin(), expected.end(), std::size_t{0});
        std::vector<std::size_t> sorted = order;
        std::sort(sorted.begin(), sorted.end());
        REQUIRE_EQ(sorted, expected);

        std::vector<FieldSpec> permuted;
        for (const std::size_t index : order) {
            permuted.push_back(fields[index]);
        }
        CHECK_EQ(compute_layout(permuted).size, brute_min_size(fields));
    }
}

TEST(2, "an allocation is aligned and inside the buffer") {
    alignas(64) std::array<std::byte, 128> storage{};
    BumpArena arena(storage);
    CHECK_EQ(arena.capacity(), 128);
    void* const first = arena.allocate(3, 1);
    REQUIRE(first != nullptr);
    void* const second = arena.allocate(8, 8);
    REQUIRE(second != nullptr);
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(second) % 8, 0);
    CHECK_GE(static_cast<std::byte*>(second), static_cast<std::byte*>(first) + 3);
    CHECK_LE(static_cast<std::byte*>(second) + 8, storage.data() + storage.size());
    CHECK_EQ(arena.used(), 16);
}

TEST(2, "padding counts against capacity exactly") {
    alignas(16) std::array<std::byte, 16> storage{};
    BumpArena arena(storage);
    CHECK(arena.allocate(1, 1) != nullptr);
    // 7 bytes of padding plus 8 bytes of payload fit exactly.
    CHECK(arena.allocate(8, 8) != nullptr);
    CHECK_EQ(arena.used(), 16);
    CHECK(arena.allocate(1, 1) == nullptr);
}

TEST(2, "a failed allocation leaves the arena usable") {
    alignas(16) std::array<std::byte, 32> storage{};
    BumpArena arena(storage);
    CHECK(arena.allocate(1024, 1) == nullptr);
    CHECK_EQ(arena.used(), 0);
    CHECK(arena.allocate(32, 16) != nullptr);
    CHECK_EQ(arena.used(), 32);
}

TEST(2, "huge sizes cannot wrap around") {
    alignas(16) std::array<std::byte, 64> storage{};
    BumpArena arena(storage);
    CHECK(arena.allocate(std::numeric_limits<std::size_t>::max(), 1) == nullptr);
    CHECK(arena.allocate(std::numeric_limits<std::size_t>::max() - 7, 8) == nullptr);
    CHECK(arena.allocate(std::numeric_limits<std::size_t>::max() / 2, 4096) == nullptr);
    CHECK_EQ(arena.used(), 0);
}

TEST(2, "alignment must be a power of two") {
    alignas(16) std::array<std::byte, 64> storage{};
    BumpArena arena(storage);
    CHECK_THROWS_AS(arena.allocate(8, 0), std::invalid_argument);
    CHECK_THROWS_AS(arena.allocate(8, 3), std::invalid_argument);
    CHECK_EQ(arena.used(), 0);
}

TEST(2, "zero-byte allocations are valid and aligned") {
    alignas(16) std::array<std::byte, 8> storage{};
    BumpArena arena(storage);
    void* const pointer = arena.allocate(0, 8);
    CHECK(pointer != nullptr);
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(pointer) % 8, 0);
}

TEST(2, "reset makes the whole buffer available again") {
    alignas(16) std::array<std::byte, 16> storage{};
    BumpArena arena(storage);
    CHECK(arena.allocate(16, 1) != nullptr);
    arena.reset();
    CHECK_EQ(arena.used(), 0);
    void* const again = arena.allocate(16, 1);
    CHECK_EQ(again, static_cast<void*>(storage.data()));
}

TEST(2, "random allocations are disjoint, aligned and in bounds") {
    Rng rng;
    alignas(64) std::array<std::byte, 8192> storage{};
    BumpArena arena(storage);
    struct Block {
        std::byte* begin;
        std::size_t bytes;
    };
    std::vector<Block> blocks;
    for (int step = 0; step < 4000; ++step) {
        const auto bytes = static_cast<std::size_t>(rng.uniform(0, 256));
        const std::size_t align = std::size_t{1} << rng.uniform(0, 6);
        INFO("step ", step, " bytes ", bytes, " align ", align);
        auto* const pointer = static_cast<std::byte*>(arena.allocate(bytes, align));
        if (pointer == nullptr) {
            arena.reset();
            blocks.clear();
            continue;
        }
        REQUIRE(pointer >= storage.data());
        REQUIRE(pointer + bytes <= storage.data() + storage.size());
        REQUIRE_EQ(reinterpret_cast<std::uintptr_t>(pointer) % align, 0);
        std::memset(pointer, 0x5A, bytes);
        for (const Block& block : blocks) {
            const bool disjoint =
                pointer + bytes <= block.begin || block.begin + block.bytes <= pointer;
            REQUIRE(disjoint || bytes == 0 || block.bytes == 0);
        }
        blocks.push_back({pointer, bytes});
    }
}

TEST(3, "construction is deferred until elements are added") {
    const auto before = Tracked::live();
    {
        StaticVector<Tracked, 8> v;
        CHECK_EQ(Tracked::live(), before);
        v.push_back(Tracked(1));
        CHECK_EQ(Tracked::live(), before + 1);
        CHECK_EQ(v.size(), 1);
        CHECK_EQ(v[0].value(), 1);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "emplace_back works without a default constructor") {
    StaticVector<NoDefault, 2> v;
    CHECK_EQ(v.emplace_back(7).value, 7);
    CHECK_EQ(v.push_back(NoDefault(8)).value, 8);
    CHECK_EQ(v[1].value, 8);
}

TEST(3, "full and empty misuse throw and leave the vector intact") {
    StaticVector<int, 2> v;
    CHECK_THROWS_AS(v.pop_back(), std::out_of_range);
    v.push_back(1);
    v.push_back(2);
    CHECK_THROWS_AS(v.push_back(3), std::length_error);
    CHECK_THROWS_AS(v.emplace_back(4), std::length_error);
    CHECK_EQ(v.size(), 2);
    CHECK_THROWS_AS(v[2], std::out_of_range);
    CHECK_EQ(v[1], 2);
}

TEST(3, "pop_back and clear destroy elements") {
    const auto before = Tracked::live();
    StaticVector<Tracked, 4> v;
    v.push_back(Tracked(1));
    v.push_back(Tracked(2));
    v.push_back(Tracked(3));
    CHECK_EQ(Tracked::live(), before + 3);
    v.pop_back();
    CHECK_EQ(v.size(), 2);
    CHECK_EQ(Tracked::live(), before + 2);
    v.clear();
    CHECK(v.empty());
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "over-aligned element types are honoured") {
    StaticVector<OverAligned, 4> v;
    v.emplace_back();
    v.emplace_back();
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(v.data()) % 64, 0);
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(&v[1]) % 64, 0);
}

TEST(3, "a throwing constructor leaves size unchanged and leaks nothing") {
    Throwing::live = 0;
    Throwing::copies = 0;
    Throwing::throw_on_copy_number = 0;
    {
        StaticVector<Throwing, 4> v;
        v.emplace_back();
        Throwing source;
        CHECK_THROWS_AS(v.push_back(source), std::runtime_error);
        CHECK_EQ(v.size(), 1);
    }
    Throwing::throw_on_copy_number = -1;
    CHECK_EQ(Throwing::live, 0);
}

TEST(3, "copy construction that throws midway destroys what was built") {
    Throwing::live = 0;
    Throwing::copies = 0;
    Throwing::throw_on_copy_number = 2;
    {
        StaticVector<Throwing, 4> source;
        source.emplace_back();
        source.emplace_back();
        source.emplace_back();
        CHECK_EQ(Throwing::live, 3);
        using ThrowingVector = StaticVector<Throwing, 4>;
        CHECK_THROWS_AS(ThrowingVector(source), std::runtime_error);
        CHECK_EQ(Throwing::live, 3);
    }
    Throwing::throw_on_copy_number = -1;
    CHECK_EQ(Throwing::live, 0);
}

TEST(3, "copies are independent and assignment replaces contents") {
    StaticVector<std::string, 4> a;
    a.push_back("one");
    a.push_back("two");
    StaticVector<std::string, 4> b(a);
    b[0] = "changed";
    CHECK_EQ(a[0], "one");
    CHECK_EQ(b[0], "changed");

    StaticVector<std::string, 4> c;
    c.push_back("old");
    c = a;
    CHECK_EQ(c.size(), 2);
    CHECK_EQ(c[1], "two");
    c = static_cast<StaticVector<std::string, 4>&>(c);  // self-assignment
    CHECK_EQ(c.size(), 2);
}

TEST(3, "moves transfer elements and empty the source") {
    const auto before = Tracked::live();
    {
        StaticVector<Tracked, 4> a;
        a.push_back(Tracked(1));
        a.push_back(Tracked(2));
        StaticVector<Tracked, 4> b(std::move(a));
        CHECK_EQ(a.size(), 0);  // NOLINT: the moved-from state is part of this class's contract
        CHECK_EQ(b.size(), 2);
        CHECK_EQ(b[1].value(), 2);

        StaticVector<Tracked, 4> c;
        c.push_back(Tracked(9));
        c = std::move(b);
        CHECK_EQ(b.size(), 0);  // NOLINT
        CHECK_EQ(c.size(), 2);
        CHECK_EQ(c[0].value(), 1);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(3, "iteration covers exactly the live elements") {
    StaticVector<int, 8> v;
    for (int i = 0; i < 5; ++i) {
        v.push_back(i * i);
    }
    CHECK_EQ(std::accumulate(v.begin(), v.end(), 0), 30);
    const StaticVector<int, 8>& view = v;
    CHECK_EQ(view.end() - view.begin(), 5);
    CHECK_EQ(view.data()[2], 4);
}

TEST(3, "random pushes and pops match std::vector") {
    Rng rng;
    const auto before = Tracked::live();
    {
        StaticVector<Tracked, 32> v;
        std::vector<int> model;
        for (int step = 0; step < 20'000; ++step) {
            INFO("step ", step);
            if (model.size() < 32 && (model.empty() || rng.coin(0.55))) {
                const int value = static_cast<int>(rng.uniform(-1000, 1000));
                v.push_back(Tracked(value));
                model.push_back(value);
            } else {
                REQUIRE_EQ(v[v.size() - 1].value(), model.back());
                v.pop_back();
                model.pop_back();
            }
            REQUIRE_EQ(v.size(), model.size());
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(4, "checked_add detects overflow in both directions") {
    constexpr int kMax = std::numeric_limits<int>::max();
    constexpr int kMin = std::numeric_limits<int>::min();
    CHECK_EQ(checked_add(1, 2), std::optional<int>(3));
    CHECK_EQ(checked_add(kMax, 0), std::optional<int>(kMax));
    CHECK_EQ(checked_add(kMax, 1), std::optional<int>());
    CHECK_EQ(checked_add(kMin, -1), std::optional<int>());
    CHECK_EQ(checked_add(kMin, kMax), std::optional<int>(-1));
}

TEST(4, "checked_mul handles the negative-one trap") {
    constexpr int kMax = std::numeric_limits<int>::max();
    constexpr int kMin = std::numeric_limits<int>::min();
    CHECK_EQ(checked_mul(46340, 46340), std::optional<int>(2147395600));
    CHECK_EQ(checked_mul(46341, 46341), std::optional<int>());
    CHECK_EQ(checked_mul(kMin, -1), std::optional<int>());
    CHECK_EQ(checked_mul(-1, kMin), std::optional<int>());
    CHECK_EQ(checked_mul(kMax, -1), std::optional<int>(-kMax));
    CHECK_EQ(checked_mul(0, kMin), std::optional<int>(0));
}

TEST(4, "checked arithmetic agrees with 64-bit math on random inputs") {
    Rng rng;
    const auto limit = static_cast<std::int64_t>(std::numeric_limits<int>::max());
    for (int step = 0; step < 100'000; ++step) {
        const int a = static_cast<int>(rng.uniform(std::numeric_limits<int>::min(), limit));
        const int b = static_cast<int>(rng.uniform(std::numeric_limits<int>::min(), limit));
        const std::int64_t sum = std::int64_t{a} + b;
        const std::int64_t product = std::int64_t{a} * b;
        INFO("a ", a, " b ", b);
        const bool sum_fits = sum >= std::numeric_limits<int>::min() && sum <= limit;
        const bool product_fits = product >= std::numeric_limits<int>::min() && product <= limit;
        REQUIRE_EQ(checked_add(a, b),
                   sum_fits ? std::optional<int>(static_cast<int>(sum)) : std::optional<int>());
        REQUIRE_EQ(checked_mul(a, b), product_fits ? std::optional<int>(static_cast<int>(product))
                                                   : std::optional<int>());
    }
}

TEST(4, "midpoint_floor never overflows and rounds toward negative infinity") {
    constexpr int kMax = std::numeric_limits<int>::max();
    constexpr int kMin = std::numeric_limits<int>::min();
    CHECK_EQ(midpoint_floor(kMax, kMax), kMax);
    CHECK_EQ(midpoint_floor(kMin, kMin), kMin);
    CHECK_EQ(midpoint_floor(kMin, kMax), -1);
    CHECK_EQ(midpoint_floor(kMax, kMin), -1);
    CHECK_EQ(midpoint_floor(-3, 0), -2);
    CHECK_EQ(midpoint_floor(-3, 2), -1);
    CHECK_EQ(midpoint_floor(3, 0), 1);
    CHECK_EQ(midpoint_floor(kMax, kMax - 1), kMax - 1);
    Rng rng;
    const auto limit = static_cast<std::int64_t>(kMax);
    for (int step = 0; step < 100'000; ++step) {
        const auto a = static_cast<int>(rng.uniform(kMin, limit));
        const auto b = static_cast<int>(rng.uniform(kMin, limit));
        const std::int64_t sum = std::int64_t{a} + b;
        const std::int64_t expected = (sum - ((sum % 2 + 2) % 2)) / 2;
        INFO("a ", a, " b ", b);
        REQUIRE_EQ(midpoint_floor(a, b), expected);
    }
}

TEST(4, "rotl32 handles zero, full turns and negative amounts") {
    const std::uint32_t x = 0x8000'0001U;
    CHECK_EQ(rotl32(x, 0), x);
    CHECK_EQ(rotl32(x, 32), x);
    CHECK_EQ(rotl32(x, -32), x);
    CHECK_EQ(rotl32(x, 1), 0x0000'0003U);
    CHECK_EQ(rotl32(x, -1), 0xC000'0000U);
    CHECK_EQ(rotl32(x, 33), 0x0000'0003U);
    CHECK_EQ(rotl32(x, std::numeric_limits<int>::min()), x);
    CHECK_EQ(rotl32(0, 7), 0);
}

TEST(4, "sign_extend covers one bit through the full width") {
    CHECK_EQ(sign_extend(1, 1), -1);
    CHECK_EQ(sign_extend(0, 1), 0);
    CHECK_EQ(sign_extend(0xFFU, 8), -1);
    CHECK_EQ(sign_extend(0x7FU, 8), 127);
    CHECK_EQ(sign_extend(0x80U, 8), -128);
    CHECK_EQ(sign_extend(0xFFFF'FFFFU, 32), -1);
    CHECK_EQ(sign_extend(0x7FFF'FFFFU, 32), 2147483647);
    CHECK_EQ(sign_extend(0xFF80U, 8), -128);  // bits above `bits` are ignored
}

TEST(4, "unaligned load and store round trip") {
    std::array<std::byte, 16> storage{};
    std::byte* const misaligned = storage.data() + 1;
    store_le32(misaligned, 0xFEDC'BA98U);
    CHECK_EQ(load_le32(misaligned), 0xFEDC'BA98U);
    CHECK_EQ(std::to_integer<int>(storage[1]), 0x98);
    CHECK_EQ(std::to_integer<int>(storage[4]), 0xFE);
    CHECK_EQ(std::to_integer<int>(storage[0]), 0);
    CHECK_EQ(std::to_integer<int>(storage[5]), 0);
    store_le32(storage.data() + 3, 0xFFFF'FFFFU);
    CHECK_EQ(load_le32(storage.data() + 3), 0xFFFF'FFFFU);
}

TEST(4, "swap works including when both references are the same object") {
    int a = 3;
    int b = -4;
    no_temp_swap(a, b);
    CHECK_EQ(a, -4);
    CHECK_EQ(b, 3);
    no_temp_swap(a, a);
    CHECK_EQ(a, -4);
    int same = std::numeric_limits<int>::min();
    no_temp_swap(same, same);
    CHECK_EQ(same, std::numeric_limits<int>::min());
}
