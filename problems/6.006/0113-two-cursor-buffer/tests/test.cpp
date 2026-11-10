#include <cstddef>
#include <deque>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "00_counters.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using practice6006::Probe;
using Buffer = TwoCursorBuffer<int>;

namespace {

std::vector<int> iota_vector(std::size_t n) {
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    return values;
}

template <typename T>
std::vector<int> contents(const TwoCursorBuffer<T>& buffer) {
    std::vector<int> out;
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        if constexpr (requires(const T& item) { item.value(); }) {
            out.push_back(buffer.read(i).value());
        } else {
            out.push_back(buffer.read(i));
        }
    }
    return out;
}

// Reference model operating on a plain vector and two gap positions.
struct Model {
    std::vector<int> items;
    std::size_t a = 0;
    std::size_t b = 0;

    void move_item(Cursor c) {
        if (c == Cursor::A) {
            const int item = items[a - 1];
            items.erase(items.begin() + static_cast<std::ptrdiff_t>(a - 1));
            items.insert(items.begin() + static_cast<std::ptrdiff_t>(b - 1), item);
            --a;
        } else {
            const int item = items[b - 1];
            items.erase(items.begin() + static_cast<std::ptrdiff_t>(b - 1));
            items.insert(items.begin() + static_cast<std::ptrdiff_t>(a), item);
            ++a;
        }
    }

    void insert_before(Cursor c, int item) {
        if (c == Cursor::A) {
            items.insert(items.begin() + static_cast<std::ptrdiff_t>(a), item);
            ++a;
            ++b;
        } else {
            items.insert(items.begin() + static_cast<std::ptrdiff_t>(b), item);
            ++b;
        }
    }

    int delete_before(Cursor c) {
        if (c == Cursor::A) {
            const int item = items[a - 1];
            items.erase(items.begin() + static_cast<std::ptrdiff_t>(a - 1));
            --a;
            --b;
            return item;
        }
        const int item = items[b - 1];
        items.erase(items.begin() + static_cast<std::ptrdiff_t>(b - 1));
        --b;
        return item;
    }
};

void expect_equal(const Buffer& buffer, const Model& model) {
    REQUIRE_EQ(contents(buffer), model.items);
    REQUIRE_EQ(buffer.cursor(Cursor::A), model.a);
    REQUIRE_EQ(buffer.cursor(Cursor::B), model.b);
}

}  // namespace

TEST(1, "building preserves order and copies the items") {
    std::vector<int> items = {5, 6, 7};
    Buffer buffer(items);
    CHECK_EQ(buffer.size(), 3);
    CHECK_EQ(contents(buffer), std::vector<int>{5, 6, 7});
    items[0] = 99;
    CHECK_EQ(buffer.read(0), 5);
}

TEST(1, "read outside the buffer throws out_of_range") {
    const std::vector<int> items = {1, 2};
    Buffer buffer(items);
    CHECK_THROWS_AS(buffer.read(2), std::out_of_range);
    const std::vector<int> none;
    Buffer empty(none);
    CHECK_EQ(empty.size(), 0);
    CHECK_THROWS_AS(empty.read(0), std::out_of_range);
}

TEST(1, "cursors report their gap once placed") {
    const std::vector<int> items = {1, 2, 3, 4};
    Buffer buffer(items);
    CHECK_THROWS_AS(buffer.cursor(Cursor::A), std::logic_error);
    buffer.place_cursor(Cursor::A, 1);
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    CHECK_THROWS_AS(buffer.cursor(Cursor::B), std::logic_error);
    buffer.place_cursor(Cursor::B, 3);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
    buffer.place_cursor(Cursor::A, 2);
    CHECK_EQ(buffer.cursor(Cursor::A), 2);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2, 3, 4});
}

TEST(1, "cursors may sit at the very ends") {
    const std::vector<int> items = {1, 2};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 0);
    buffer.place_cursor(Cursor::B, 2);
    CHECK_EQ(buffer.cursor(Cursor::A), 0);
    CHECK_EQ(buffer.cursor(Cursor::B), 2);
}

TEST(1, "invalid placements throw and change nothing") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    CHECK_THROWS_AS(buffer.place_cursor(Cursor::A, 4), std::logic_error);
    buffer.place_cursor(Cursor::A, 2);
    CHECK_THROWS_AS(buffer.place_cursor(Cursor::B, 2), std::logic_error);
    CHECK_THROWS_AS(buffer.place_cursor(Cursor::B, 1), std::logic_error);
    buffer.place_cursor(Cursor::B, 3);
    CHECK_THROWS_AS(buffer.place_cursor(Cursor::A, 3), std::logic_error);
    CHECK_EQ(buffer.cursor(Cursor::A), 2);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
}

TEST(1, "read is unaffected by cursor placement anywhere") {
    Rng rng;
    const std::vector<int> items = iota_vector(40);
    for (int round = 0; round < 200; ++round) {
        Buffer buffer(items);
        const auto a = static_cast<std::size_t>(rng.uniform(0, 39));
        const auto b = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(a) + 1, 40));
        buffer.place_cursor(Cursor::B, b);
        buffer.place_cursor(Cursor::A, a);
        INFO("a=", a, " b=", b);
        REQUIRE_EQ(contents(buffer), items);
    }
}

TEST(2, "shifting moves the gap one position") {
    const std::vector<int> items = {1, 2, 3, 4, 5};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    buffer.place_cursor(Cursor::B, 4);
    buffer.shift_cursor(Cursor::A, +1);
    CHECK_EQ(buffer.cursor(Cursor::A), 2);
    buffer.shift_cursor(Cursor::B, -1);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
    buffer.shift_cursor(Cursor::A, -1);
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    buffer.shift_cursor(Cursor::B, +1);
    CHECK_EQ(buffer.cursor(Cursor::B), 4);
    CHECK_EQ(contents(buffer), items);
}

TEST(2, "shifting requires both cursors") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::A, 1), std::logic_error);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::B, 1), std::logic_error);
}

TEST(2, "shifting past the ends or into the other cursor throws") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 0);
    buffer.place_cursor(Cursor::B, 1);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::A, -1), std::logic_error);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::A, +1), std::logic_error);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::B, -1), std::logic_error);
    buffer.shift_cursor(Cursor::B, +1);
    buffer.shift_cursor(Cursor::B, +1);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::B, +1), std::logic_error);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::A, 2), std::logic_error);
    CHECK_THROWS_AS(buffer.shift_cursor(Cursor::A, 0), std::logic_error);
    CHECK_EQ(buffer.cursor(Cursor::A), 0);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
    CHECK_EQ(contents(buffer), items);
}

TEST(2, "random shifts keep the contents and match the model") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(2, 30));
        const auto items = iota_vector(n);
        Buffer buffer(items);
        Model model{items, 0, 0};
        model.a = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        model.b = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(model.a) + 1, static_cast<std::int64_t>(n)));
        buffer.place_cursor(Cursor::A, model.a);
        buffer.place_cursor(Cursor::B, model.b);
        for (int step = 0; step < 300; ++step) {
            const Cursor c = rng.coin() ? Cursor::A : Cursor::B;
            const int d = rng.coin() ? 1 : -1;
            std::size_t a = model.a;
            std::size_t b = model.b;
            bool valid = true;
            if (c == Cursor::A && d == 1) {
                valid = a + 1 < b;
                if (valid) a = a + 1;
            } else if (c == Cursor::A) {
                valid = a >= 1;
                if (valid) a = a - 1;
            } else if (d == 1) {
                valid = b + 1 <= n;
                if (valid) b = b + 1;
            } else {
                valid = b - 1 > a;
                if (valid) b = b - 1;
            }
            INFO("round ", round, " step ", step);
            if (valid) {
                buffer.shift_cursor(c, d);
                model.a = a;
                model.b = b;
            } else {
                CHECK_THROWS_AS(buffer.shift_cursor(c, d), std::logic_error);
            }
            REQUIRE_EQ(buffer.read(model.a < n ? model.a : model.a - 1),
                       model.items[model.a < n ? model.a : model.a - 1]);
        }
        expect_equal(buffer, model);
    }
}

TEST(3, "move_item moves the line before A to just before B") {
    const std::vector<int> items = {1, 2, 3, 4, 5, 6};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 2);
    buffer.place_cursor(Cursor::B, 5);
    buffer.move_item(Cursor::A);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 3, 4, 5, 2, 6});
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    CHECK_EQ(buffer.cursor(Cursor::B), 5);
}

TEST(3, "move_item moves the line before B to just before A") {
    const std::vector<int> items = {1, 2, 3, 4, 5, 6};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 2);
    buffer.place_cursor(Cursor::B, 5);
    buffer.move_item(Cursor::B);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2, 5, 3, 4, 6});
    CHECK_EQ(buffer.cursor(Cursor::A), 3);
    CHECK_EQ(buffer.cursor(Cursor::B), 5);
}

TEST(3, "move_item requires both cursors and a line to move") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::B, 2);
    CHECK_THROWS_AS(buffer.move_item(Cursor::B), std::logic_error);
    buffer.place_cursor(Cursor::A, 0);
    CHECK_THROWS_AS(buffer.move_item(Cursor::A), std::logic_error);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2, 3});
}

TEST(3, "move_item of B is rejected when it would make the cursors touch") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    buffer.place_cursor(Cursor::B, 2);
    CHECK_THROWS_AS(buffer.move_item(Cursor::B), std::logic_error);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2, 3});
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
}

TEST(3, "moving everything back and forth") {
    const auto items = iota_vector(5);
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 2);
    buffer.place_cursor(Cursor::B, 3);
    buffer.move_item(Cursor::A);  // 0 2 1 3 4, A=1 B=3
    buffer.move_item(Cursor::A);  // 2 1 0 3 4, A=0 B=3
    CHECK_EQ(contents(buffer), std::vector<int>{2, 1, 0, 3, 4});
    CHECK_EQ(buffer.cursor(Cursor::A), 0);
    buffer.move_item(Cursor::B);  // 0 2 1 3 4, A=1 B=3
    CHECK_EQ(contents(buffer), std::vector<int>{0, 2, 1, 3, 4});
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    buffer.move_item(Cursor::B);  // 0 1 2 3 4, A=2 B=3
    CHECK_EQ(contents(buffer), items);
}

TEST(3, "random moves, shifts and placements match the model") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(2, 25));
        Model model{iota_vector(n), 0, 0};
        Buffer buffer(model.items);
        model.a = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        model.b = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(model.a) + 1, static_cast<std::int64_t>(n)));
        buffer.place_cursor(Cursor::A, model.a);
        buffer.place_cursor(Cursor::B, model.b);
        for (int step = 0; step < 400; ++step) {
            INFO("round ", round, " step ", step);
            const Cursor c = rng.coin() ? Cursor::A : Cursor::B;
            const auto op = rng.uniform(0, 9);
            if (op <= 6) {
                const bool valid = c == Cursor::A ? model.a >= 1 : model.b - model.a >= 2;
                if (valid) {
                    buffer.move_item(c);
                    model.move_item(c);
                } else {
                    CHECK_THROWS_AS(buffer.move_item(c), std::logic_error);
                }
            } else if (op <= 8) {
                const int d = rng.coin() ? 1 : -1;
                std::size_t a = model.a;
                std::size_t b = model.b;
                bool valid;
                if (c == Cursor::A && d == 1) {
                    valid = a + 1 < b;
                    a = a + 1;
                } else if (c == Cursor::A) {
                    valid = a >= 1;
                    a = a - 1;
                } else if (d == 1) {
                    valid = b + 1 <= n;
                    b = b + 1;
                } else {
                    valid = b - 1 > a;
                    b = b - 1;
                }
                if (valid) {
                    buffer.shift_cursor(c, d);
                    model.a = a;
                    model.b = b;
                } else {
                    CHECK_THROWS_AS(buffer.shift_cursor(c, d), std::logic_error);
                }
            } else {
                const std::size_t a = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
                const std::size_t b = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(a) + 1, static_cast<std::int64_t>(n)));
                if (c == Cursor::A) {
                    if (a < model.b) {
                        buffer.place_cursor(Cursor::A, a);
                        model.a = a;
                    }
                } else if (b > model.a) {
                    buffer.place_cursor(Cursor::B, b);
                    model.b = b;
                }
            }
            REQUIRE_EQ(buffer.read(0), model.items[0]);
            REQUIRE_EQ(buffer.read(n - 1), model.items[n - 1]);
        }
        expect_equal(buffer, model);
    }
}

TEST(3, "element writes are amortized constant under the worst repeated moves") {
    using Item = Probe<int>;
    const std::size_t n = 1'200;
    std::vector<Item> items;
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(static_cast<int>(i));
    }
    TwoCursorBuffer<Item> buffer(items);
    buffer.place_cursor(Cursor::A, n / 3);
    buffer.place_cursor(Cursor::B, 2 * n / 3);

    Item::reset();
    std::size_t operations = 0;
    for (int round = 0; round < 12; ++round) {
        while (buffer.cursor(Cursor::A) > 0) {
            buffer.move_item(Cursor::A);
            ++operations;
        }
        while (buffer.cursor(Cursor::B) - buffer.cursor(Cursor::A) >= 2) {
            buffer.move_item(Cursor::B);
            ++operations;
        }
    }
    CHECK_LE(Item::writes(), 10 * operations + 8 * n);
}

TEST(3, "element writes stay small when the middle segment is large") {
    using Item = Probe<int>;
    const std::size_t n = 1'000;
    std::vector<Item> items;
    for (std::size_t i = 0; i < n; ++i) {
        items.emplace_back(static_cast<int>(i));
    }
    TwoCursorBuffer<Item> buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    buffer.place_cursor(Cursor::B, n - 1);
    // Each operation here would shift about n elements in a flat array.
    Item::reset();
    for (int round = 0; round < 500; ++round) {
        buffer.move_item(Cursor::A);
        buffer.move_item(Cursor::B);
    }
    CHECK_LE(Item::writes(), 10 * 1'000 + 8 * n);
}

// A single flat array rewrites about half of the buffer per operation here.
PERF(3, "constant-time reads and moves on a million lines", 3000) {
    const auto items = iota_vector(1'000'000);
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 300'000);
    buffer.place_cursor(Cursor::B, 700'000);
    Rng rng;
    long long sum = 0;
    for (int i = 0; i < 2'000'000; ++i) {
        if (i % 3 == 0) {
            buffer.move_item(Cursor::A);
        } else if (i % 3 == 1) {
            buffer.move_item(Cursor::B);
        } else {
            sum += buffer.read(static_cast<std::size_t>(rng.uniform(0, 999'999)));
        }
    }
    CHECK_GT(sum, 0);
}

TEST(4, "inserting before cursor A shifts both cursors") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    buffer.place_cursor(Cursor::B, 2);
    buffer.insert_before_cursor(Cursor::A, 9);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 9, 2, 3});
    CHECK_EQ(buffer.cursor(Cursor::A), 2);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
}

TEST(4, "inserting before cursor B shifts only B") {
    const std::vector<int> items = {1, 2, 3};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 1);
    buffer.place_cursor(Cursor::B, 2);
    buffer.insert_before_cursor(Cursor::B, 9);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2, 9, 3});
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
}

TEST(4, "deleting returns the line and adjusts the cursors") {
    const std::vector<int> items = {1, 2, 3, 4, 5};
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 2);
    buffer.place_cursor(Cursor::B, 4);
    CHECK_EQ(buffer.delete_before_cursor(Cursor::A), 2);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 3, 4, 5});
    CHECK_EQ(buffer.cursor(Cursor::A), 1);
    CHECK_EQ(buffer.cursor(Cursor::B), 3);
    CHECK_EQ(buffer.delete_before_cursor(Cursor::B), 4);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 3, 5});
    CHECK_EQ(buffer.cursor(Cursor::B), 2);
}

TEST(4, "invalid edits throw and change nothing") {
    const std::vector<int> items = {1, 2};
    Buffer buffer(items);
    CHECK_THROWS_AS(buffer.insert_before_cursor(Cursor::A, 1), std::logic_error);
    buffer.place_cursor(Cursor::A, 0);
    buffer.place_cursor(Cursor::B, 1);
    CHECK_THROWS_AS(buffer.delete_before_cursor(Cursor::A), std::logic_error);
    CHECK_THROWS_AS(buffer.delete_before_cursor(Cursor::B), std::logic_error);
    CHECK_EQ(contents(buffer), std::vector<int>{1, 2});
    CHECK_EQ(buffer.cursor(Cursor::A), 0);
    CHECK_EQ(buffer.cursor(Cursor::B), 1);
}

TEST(4, "lines are released") {
    const auto before = Tracked::live();
    {
        std::vector<Tracked> items(6);
        TwoCursorBuffer<Tracked> buffer(items);
        buffer.place_cursor(Cursor::A, 2);
        buffer.place_cursor(Cursor::B, 4);
        for (int i = 0; i < 100; ++i) {
            buffer.insert_before_cursor(i % 2 == 0 ? Cursor::A : Cursor::B, Tracked(i));
        }
        for (int i = 0; i < 40; ++i) {
            buffer.delete_before_cursor(i % 2 == 0 ? Cursor::A : Cursor::B);
        }
        buffer.move_item(Cursor::A);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(4, "random edits match the model") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(2, 20));
        Model model{iota_vector(n), 0, 0};
        Buffer buffer(model.items);
        model.a = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
        model.b = static_cast<std::size_t>(rng.uniform(static_cast<std::int64_t>(model.a) + 1, static_cast<std::int64_t>(n)));
        buffer.place_cursor(Cursor::A, model.a);
        buffer.place_cursor(Cursor::B, model.b);
        int next_value = 1'000;
        for (int step = 0; step < 300; ++step) {
            INFO("round ", round, " step ", step);
            const Cursor c = rng.coin() ? Cursor::A : Cursor::B;
            const auto op = rng.uniform(0, 3);
            if (op == 0) {
                buffer.insert_before_cursor(c, next_value);
                model.insert_before(c, next_value);
                ++next_value;
            } else if (op == 1) {
                const bool valid = c == Cursor::A ? model.a >= 1 : model.b - model.a >= 2;
                if (valid) {
                    REQUIRE_EQ(buffer.delete_before_cursor(c), model.delete_before(c));
                } else {
                    CHECK_THROWS_AS(buffer.delete_before_cursor(c), std::logic_error);
                }
            } else if (op == 2) {
                const bool valid = c == Cursor::A ? model.a >= 1 : model.b - model.a >= 2;
                if (valid) {
                    buffer.move_item(c);
                    model.move_item(c);
                } else {
                    CHECK_THROWS_AS(buffer.move_item(c), std::logic_error);
                }
            } else {
                const int d = rng.coin() ? 1 : -1;
                bool valid;
                std::size_t a = model.a;
                std::size_t b = model.b;
                if (c == Cursor::A && d == 1) {
                    valid = a + 1 < b;
                    a = a + 1;
                } else if (c == Cursor::A) {
                    valid = a >= 1;
                    a = a - 1;
                } else if (d == 1) {
                    valid = b + 1 <= model.items.size();
                    b = b + 1;
                } else {
                    valid = b - 1 > a;
                    b = b - 1;
                }
                if (valid) {
                    buffer.shift_cursor(c, d);
                    model.a = a;
                    model.b = b;
                } else {
                    CHECK_THROWS_AS(buffer.shift_cursor(c, d), std::logic_error);
                }
            }
            expect_equal(buffer, model);
        }
    }
}

PERF(4, "insert and delete at the cursors are constant time", 3000) {
    const auto items = iota_vector(1'000'000);
    Buffer buffer(items);
    buffer.place_cursor(Cursor::A, 200'000);
    buffer.place_cursor(Cursor::B, 800'000);
    for (int i = 0; i < 3'000'000; ++i) {
        const Cursor c = i % 2 == 0 ? Cursor::A : Cursor::B;
        buffer.insert_before_cursor(c, i);
        if (i % 4 == 0) {
            buffer.delete_before_cursor(c);
        }
    }
    CHECK_GT(buffer.size(), 1'000'000);
}
