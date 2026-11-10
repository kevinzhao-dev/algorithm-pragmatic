#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

enum class Cursor { A, B };

// Three segments: before_a_ (lines before A), between_ (between A and B), and after_b_ (lines
// after B, stored reversed so that both of B's neighbors live at container ends). A vector or a
// deque supports O(1) access to its ends and by index, so every operation is amortized O(1).
template <typename T>
class TwoCursorBuffer {
public:
    explicit TwoCursorBuffer(std::span<const T> items) : before_a_(items.begin(), items.end()) {}

    std::size_t size() const { return before_a_.size() + between_.size() + after_b_.size(); }

    const T& read(std::size_t i) const {
        if (i >= size()) {
            throw std::out_of_range("line index out of range");
        }
        if (i < before_a_.size()) {
            return before_a_[i];
        }
        i -= before_a_.size();
        if (i < between_.size()) {
            return between_[i];
        }
        i -= between_.size();
        return after_b_[after_b_.size() - 1 - i];
    }

    void place_cursor(Cursor c, std::size_t gap) {
        if (gap > size()) {
            throw std::out_of_range("cursor gap out of range");
        }
        std::optional<std::size_t> a = gap_a_;
        std::optional<std::size_t> b = gap_b_;
        (c == Cursor::A ? a : b) = gap;
        if (a && b && *a >= *b) {
            throw std::logic_error("cursor A must stay strictly before cursor B");
        }
        gap_a_ = a;
        gap_b_ = b;
        if (gap_a_ && gap_b_) {
            redistribute();
        }
    }

    std::size_t cursor(Cursor c) const {
        const auto& gap = c == Cursor::A ? gap_a_ : gap_b_;
        if (!gap) {
            throw std::logic_error("cursor has not been placed");
        }
        return *gap;
    }

    void shift_cursor(Cursor c, int direction) {
        require_placed();
        if (direction != 1 && direction != -1) {
            throw std::logic_error("direction must be +1 or -1");
        }
        const std::size_t a = *gap_a_;
        const std::size_t b = *gap_b_;
        if (c == Cursor::A && direction == 1) {
            require(a + 1 < b, "cursors would touch");
            before_a_.push_back(std::move(between_.front()));
            between_.pop_front();
            gap_a_ = a + 1;
        } else if (c == Cursor::A) {
            require(a >= 1, "cursor A is at the start");
            between_.push_front(std::move(before_a_.back()));
            before_a_.pop_back();
            gap_a_ = a - 1;
        } else if (direction == 1) {
            require(b + 1 <= size(), "cursor B is at the end");
            between_.push_back(std::move(after_b_.back()));
            after_b_.pop_back();
            gap_b_ = b + 1;
        } else {
            require(b - 1 > a, "cursors would touch");
            after_b_.push_back(std::move(between_.back()));
            between_.pop_back();
            gap_b_ = b - 1;
        }
    }

    void move_item(Cursor c) {
        require_placed();
        if (c == Cursor::A) {
            require(*gap_a_ >= 1, "no line before cursor A");
            between_.push_back(std::move(before_a_.back()));
            before_a_.pop_back();
            --*gap_a_;
        } else {
            require(*gap_b_ - *gap_a_ >= 2, "moving this line would make the cursors touch");
            before_a_.push_back(std::move(between_.back()));
            between_.pop_back();
            ++*gap_a_;
        }
    }

    void insert_before_cursor(Cursor c, T item) {
        require_placed();
        if (c == Cursor::A) {
            before_a_.push_back(std::move(item));
            ++*gap_a_;
            ++*gap_b_;
        } else {
            between_.push_back(std::move(item));
            ++*gap_b_;
        }
    }

    T delete_before_cursor(Cursor c) {
        require_placed();
        T item = [&] {
            if (c == Cursor::A) {
                require(*gap_a_ >= 1, "no line before cursor A");
                return take_back(before_a_);
            }
            require(*gap_b_ - *gap_a_ >= 2, "deleting this line would make the cursors touch");
            return take_back(between_);
        }();
        if (c == Cursor::A) {
            --*gap_a_;
        }
        --*gap_b_;
        return item;
    }

private:
    template <typename Container>
    static T take_back(Container& container) {
        T item = std::move(container.back());
        container.pop_back();
        return item;
    }

    static void require(bool condition, const char* message) {
        if (!condition) {
            throw std::logic_error(message);
        }
    }

    void require_placed() const {
        require(gap_a_ && gap_b_, "both cursors must be placed");
    }

    // Rearranges storage into the three segments for the current gaps. Called only from
    // place_cursor, which is allowed to take O(n).
    void redistribute() {
        std::vector<T> all;
        all.reserve(size());
        for (T& item : before_a_) {
            all.push_back(std::move(item));
        }
        for (T& item : between_) {
            all.push_back(std::move(item));
        }
        for (auto it = after_b_.rbegin(); it != after_b_.rend(); ++it) {
            all.push_back(std::move(*it));
        }
        before_a_.clear();
        between_.clear();
        after_b_.clear();

        const std::size_t a = *gap_a_;
        const std::size_t b = *gap_b_;
        for (std::size_t i = 0; i < a; ++i) {
            before_a_.push_back(std::move(all[i]));
        }
        for (std::size_t i = a; i < b; ++i) {
            between_.push_back(std::move(all[i]));
        }
        for (std::size_t i = all.size(); i > b; --i) {
            after_b_.push_back(std::move(all[i - 1]));
        }
    }

    std::vector<T> before_a_;
    std::deque<T> between_;
    std::vector<T> after_b_;
    std::optional<std::size_t> gap_a_;
    std::optional<std::size_t> gap_b_;
};
