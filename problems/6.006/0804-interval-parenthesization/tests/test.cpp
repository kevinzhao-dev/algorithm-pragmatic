#include <cctype>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Operands = std::vector<std::int64_t>;

// Every value any parenthesization of operands[first, last) can produce.
std::set<std::int64_t> all_values(const Operands& operands, std::string_view ops, std::size_t first,
                                  std::size_t last) {
    if (last - first == 1) {
        return {operands[first]};
    }
    std::set<std::int64_t> values;
    for (std::size_t split = first + 1; split < last; ++split) {
        for (const std::int64_t a : all_values(operands, ops, first, split)) {
            for (const std::int64_t b : all_values(operands, ops, split, last)) {
                values.insert(ops[split - 1] == '+' ? a + b : a * b);
            }
        }
    }
    return values;
}

std::int64_t brute_max(const Operands& operands, std::string_view ops) {
    return *all_values(operands, ops, 0, operands.size()).rbegin();
}

std::string random_ops(Rng& rng, std::size_t count) {
    return rng.text(count, "+*");
}

// A small parser for the output format; records the leaves and operators it sees, in order.
class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    std::int64_t parse() {
        const std::int64_t value = expression();
        REQUIRE_EQ(at_, text_.size());
        return value;
    }

    std::vector<std::int64_t> leaves;
    std::string operators;

private:
    std::int64_t expression() {
        REQUIRE(at_ < text_.size());
        if (text_[at_] == '(') {
            ++at_;
            const std::int64_t left = expression();
            REQUIRE(at_ < text_.size());
            const char op = text_[at_++];
            operators += op;
            const std::int64_t right = expression();
            REQUIRE(at_ < text_.size());
            REQUIRE_EQ(text_[at_], ')');
            ++at_;
            REQUIRE(op == '+' || op == '*');
            return op == '+' ? left + right : left * right;
        }
        bool negative = false;
        if (text_[at_] == '-') {
            negative = true;
            ++at_;
        }
        REQUIRE(at_ < text_.size());
        REQUIRE(std::isdigit(static_cast<unsigned char>(text_[at_])));
        std::int64_t value = 0;
        while (at_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[at_]))) {
            value = value * 10 + (text_[at_++] - '0');
        }
        if (negative) {
            value = -value;
        }
        leaves.push_back(value);
        return value;
    }

    std::string_view text_;
    std::size_t at_ = 0;
};

std::int64_t brute_chain(const std::vector<std::int64_t>& dims, std::size_t first,
                         std::size_t last) {
    if (first == last) {
        return 0;
    }
    std::int64_t best = std::numeric_limits<std::int64_t>::max();
    for (std::size_t cut = first; cut < last; ++cut) {
        best = std::min(best, brute_chain(dims, first, cut) + brute_chain(dims, cut + 1, last) +
                                  dims[first] * dims[cut + 1] * dims[last + 1]);
    }
    return best;
}

struct ChainNode {
    std::int64_t rows;
    std::int64_t cols;
    std::int64_t cost;
};

// Parses a multiplication order and returns its total cost; checks every matrix appears in order.
class ChainParser {
public:
    ChainParser(std::string_view text, const std::vector<std::int64_t>& dims)
        : text_(text), dims_(dims) {}

    ChainNode parse() {
        const ChainNode node = expression();
        REQUIRE_EQ(at_, text_.size());
        REQUIRE_EQ(next_matrix_, dims_.size() - 1);
        return node;
    }

private:
    ChainNode expression() {
        REQUIRE(at_ < text_.size());
        if (text_[at_] == '(') {
            ++at_;
            const ChainNode left = expression();
            REQUIRE(at_ < text_.size());
            REQUIRE_EQ(text_[at_++], '*');
            const ChainNode right = expression();
            REQUIRE(at_ < text_.size());
            REQUIRE_EQ(text_[at_++], ')');
            REQUIRE_EQ(left.cols, right.rows);
            return {left.rows, right.cols,
                    left.cost + right.cost + left.rows * left.cols * right.cols};
        }
        REQUIRE_EQ(text_[at_++], 'M');
        REQUIRE(at_ < text_.size());
        std::size_t index = 0;
        while (at_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[at_]))) {
            index = index * 10 + static_cast<std::size_t>(text_[at_++] - '0');
        }
        REQUIRE_EQ(index, next_matrix_);
        ++next_matrix_;
        return {dims_[index], dims_[index + 1], 0};
    }

    std::string_view text_;
    const std::vector<std::int64_t>& dims_;
    std::size_t at_ = 0;
    std::size_t next_matrix_ = 0;
};

}  // namespace

TEST(1, "a single operand evaluates to itself") {
    const Operands operands{7};
    CHECK_EQ(max_value(operands, ""), 7);
}

TEST(1, "two operands have only one way to combine") {
    const Operands operands{3, 4};
    CHECK_EQ(max_value(operands, "+"), 7);
    CHECK_EQ(max_value(operands, "*"), 12);
}

TEST(1, "the order of operations can matter a lot") {
    const Operands operands{2, 3, 4};
    CHECK_EQ(max_value(operands, "+*"), 20);  // (2+3)*4
}

TEST(1, "malformed expressions are rejected") {
    const Operands operands{1, 2};
    CHECK_THROWS_AS(max_value({}, ""), std::invalid_argument);
    CHECK_THROWS_AS(max_value(operands, ""), std::invalid_argument);
    CHECK_THROWS_AS(max_value(operands, "-"), std::invalid_argument);
}

TEST(1, "random non-negative expressions match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 8));
        const auto raw = rng.ints(n, 0, 9);
        const Operands operands(raw.begin(), raw.end());
        const std::string ops = random_ops(rng, n - 1);
        INFO("operands ", operands, " ops ", ops);
        REQUIRE_EQ(max_value(operands, ops), brute_max(operands, ops));
    }
}

TEST(2, "two negatives can multiply into the maximum") {
    const Operands operands{-3, 1, -4};
    // (-3 + 1) * -4 = 8, while -3 + (1 * -4) = -7.
    CHECK_EQ(max_value(operands, "+*"), 8);
}

TEST(2, "both children may need to be at their minimum") {
    const Operands operands{-1, -2, -3, -4};
    // Keeping each pair negative, then multiplying, needs the minimum of each half.
    CHECK_EQ(max_value(operands, "+*+"), 21);  // ((-1 + -2) * (-3 + -4))
}

TEST(2, "a child's minimum can feed the parent's maximum") {
    const Operands operands{-3, -3, -2, -3};
    // (-3 + (-3 * -2)) * -3 = -9, but keeping only each child's maximum finds at best -21.
    CHECK_EQ(max_value(operands, "+**"), -9);
}

TEST(2, "random signed expressions match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 8));
        const auto raw = rng.ints(n, -8, 8);
        const Operands operands(raw.begin(), raw.end());
        const std::string ops = random_ops(rng, n - 1);
        INFO("operands ", operands, " ops ", ops);
        REQUIRE_EQ(max_value(operands, ops), brute_max(operands, ops));
    }
}

PERF(2, "four hundred fifty operands in cubic time", 1500) {
    Rng rng;
    const auto n = std::size_t{450};
    const auto raw = rng.ints(n, -3, 3);
    const Operands operands(raw.begin(), raw.end());
    // Mostly additions keep every intermediate value small.
    std::string ops(n - 1, '+');
    ops[100] = '*';
    ops[200] = '*';
    ops[300] = '*';
    CHECK_GE(max_value(operands, ops), -1000);
}

TEST(3, "a single operand needs no parentheses") {
    const Operands operands{-5};
    CHECK_EQ(best_parenthesization(operands, ""), "-5");
}

TEST(3, "a single operation gets one pair of parentheses") {
    const Operands operands{1, -2};
    CHECK_EQ(best_parenthesization(operands, "+"), "(1+-2)");
}

TEST(3, "the rendered expression evaluates to the maximum") {
    const Operands operands{2, 3, 4};
    const std::string rendered = best_parenthesization(operands, "+*");
    Parser parser(rendered);
    CHECK_EQ(parser.parse(), 20);
    CHECK_EQ(parser.leaves, operands);
    CHECK_EQ(parser.operators, "+*");
}

TEST(3, "random renderings keep the order and hit the maximum") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 8));
        const auto raw = rng.ints(n, -8, 8);
        const Operands operands(raw.begin(), raw.end());
        const std::string ops = random_ops(rng, n - 1);
        INFO("operands ", operands, " ops ", ops);
        const std::string rendered = best_parenthesization(operands, ops);
        Parser parser(rendered);
        INFO("rendered ", rendered);
        REQUIRE_EQ(parser.parse(), max_value(operands, ops));
        REQUIRE_EQ(parser.leaves, operands);
        REQUIRE_EQ(parser.operators, ops);
    }
}

TEST(4, "a single matrix costs nothing") {
    const std::vector<std::int64_t> dims{7, 3};
    CHECK_EQ(min_multiplications(dims), 0);
    CHECK_EQ(best_multiplication_order(dims), "M0");
}

TEST(4, "bad dimensions are rejected") {
    const std::vector<std::int64_t> dims{4};
    CHECK_THROWS_AS(min_multiplications(dims), std::invalid_argument);
    CHECK_THROWS_AS(best_multiplication_order({}), std::invalid_argument);
}

TEST(4, "association changes the cost") {
    const std::vector<std::int64_t> dims{10, 100, 5, 50};
    CHECK_EQ(min_multiplications(dims), 10 * 100 * 5 + 10 * 5 * 50);
    CHECK_EQ(best_multiplication_order(dims), "((M0*M1)*M2)");
}

TEST(4, "random chains match enumeration and the order costs the optimum") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 8));
        const auto raw = rng.ints(n + 1, 1, 30);
        const std::vector<std::int64_t> dims(raw.begin(), raw.end());
        const std::int64_t optimal = brute_chain(dims, 0, n - 1);
        INFO("dims ", dims);
        REQUIRE_EQ(min_multiplications(dims), optimal);
        const std::string order = best_multiplication_order(dims);
        ChainParser parser(order, dims);
        INFO("order ", order);
        REQUIRE_EQ(parser.parse().cost, optimal);
    }
}

PERF(4, "nine hundred matrices in cubic time", 1500) {
    Rng rng;
    const auto raw = rng.ints(901, 1, 100);
    const std::vector<std::int64_t> dims(raw.begin(), raw.end());
    CHECK_GT(min_multiplications(dims), 0);
    CHECK_GT(best_multiplication_order(dims).size(), 900);
}
