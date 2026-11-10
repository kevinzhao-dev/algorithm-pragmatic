#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Bits are packed little-endian: bit i lives at bit (i % 64) of word i / 64.
// One spare word at the end keeps unaligned 64-bit windows inside the allocation.
class BitArray {
public:
    explicit BitArray(std::size_t bits) : bits_(bits), words_(word_count(bits), 0) {}

    static BitArray from_string(std::string_view text) {
        BitArray result(text.size());
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (text[i] != '0' && text[i] != '1') {
                throw std::invalid_argument("bit strings use only '0' and '1'");
            }
            result.set(i, text[i] == '1');
        }
        return result;
    }

    std::size_t size() const { return bits_; }

    bool get(std::size_t index) const {
        check_index(index);
        return (words_[index >> 6] >> (index & 63)) & 1;
    }

    void set(std::size_t index, bool value) {
        check_index(index);
        const std::uint64_t mask = std::uint64_t{1} << (index & 63);
        if (value) {
            words_[index >> 6] |= mask;
        } else {
            words_[index >> 6] &= ~mask;
        }
    }

    std::string to_string() const {
        std::string text(bits_, '0');
        for (std::size_t i = 0; i < bits_; ++i) {
            if ((words_[i >> 6] >> (i & 63)) & 1) {
                text[i] = '1';
            }
        }
        return text;
    }

    // Bit `offset + j` becomes bit j of the result.
    std::uint64_t read_bits(std::size_t offset, unsigned count) const {
        check_field(offset, count);
        return read_unchecked(offset, count);
    }

    // Bit j of `value` becomes bit `offset + j`; bits of `value` at or above `count` are ignored.
    void write_bits(std::size_t offset, unsigned count, std::uint64_t value) {
        check_field(offset, count);
        write_unchecked(offset, count, value);
    }

    // Rotates [offset, offset + length) toward higher indices by `right_amount`, cyclically.
    void rotate(std::size_t offset, std::size_t length, std::ptrdiff_t right_amount) {
        if (offset > bits_ || length > bits_ - offset) {
            throw std::out_of_range("rotate range is outside the array");
        }
        if (length < 2) {
            return;
        }
        // Reduce modulo length without negating right_amount (PTRDIFF_MIN has no negation).
        const auto signed_length = static_cast<std::ptrdiff_t>(length);
        std::ptrdiff_t remainder = right_amount % signed_length;
        if (remainder < 0) {
            remainder += signed_length;
        }
        const auto amount = static_cast<std::size_t>(remainder);
        if (amount == 0) {
            return;
        }
        // (ab)^R = b^R a^R, so reversing the whole range and then each part yields ba.
        reverse_range(offset, offset + length);
        reverse_range(offset, offset + amount);
        reverse_range(offset + amount, offset + length);
    }

private:
    static std::size_t word_count(std::size_t bits) { return bits / 64 + 2; }

    static std::uint64_t low_mask(unsigned count) {
        return count == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << count) - 1;
    }

    void check_index(std::size_t index) const {
        if (index >= bits_) {
            throw std::out_of_range("bit index out of range");
        }
    }

    void check_field(std::size_t offset, unsigned count) const {
        if (count > 64 || offset > bits_ || count > bits_ - offset) {
            throw std::out_of_range("bit field outside the array");
        }
    }

    std::uint64_t read_unchecked(std::size_t offset, unsigned count) const {
        if (count == 0) {
            return 0;
        }
        const std::size_t word = offset >> 6;
        const unsigned shift = offset & 63;
        std::uint64_t value = words_[word] >> shift;
        if (shift != 0) {
            value |= words_[word + 1] << (64 - shift);
        }
        return value & low_mask(count);
    }

    void write_unchecked(std::size_t offset, unsigned count, std::uint64_t value) {
        if (count == 0) {
            return;
        }
        const std::size_t word = offset >> 6;
        const unsigned shift = offset & 63;
        const std::uint64_t mask = low_mask(count);
        const std::uint64_t field = value & mask;
        words_[word] = (words_[word] & ~(mask << shift)) | (field << shift);
        if (shift != 0 && shift + count > 64) {
            const unsigned low_part = 64 - shift;
            words_[word + 1] = (words_[word + 1] & ~(mask >> low_part)) | (field >> low_part);
        }
    }

    static std::uint64_t reverse_word(std::uint64_t x) {
        x = ((x >> 1) & 0x5555555555555555ULL) | ((x & 0x5555555555555555ULL) << 1);
        x = ((x >> 2) & 0x3333333333333333ULL) | ((x & 0x3333333333333333ULL) << 2);
        x = ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((x & 0x0F0F0F0F0F0F0F0FULL) << 4);
        x = ((x >> 8) & 0x00FF00FF00FF00FFULL) | ((x & 0x00FF00FF00FF00FFULL) << 8);
        x = ((x >> 16) & 0x0000FFFF0000FFFFULL) | ((x & 0x0000FFFF0000FFFFULL) << 16);
        return (x >> 32) | (x << 32);
    }

    // Reverses a `width`-bit value.
    static std::uint64_t reverse_field(std::uint64_t value, unsigned width) {
        return width == 0 ? 0 : reverse_word(value) >> (64 - width);
    }

    void reverse_range(std::size_t begin, std::size_t end) {
        while (end - begin >= 128) {
            const std::uint64_t left = read_unchecked(begin, 64);
            const std::uint64_t right = read_unchecked(end - 64, 64);
            write_unchecked(begin, 64, reverse_word(right));
            write_unchecked(end - 64, 64, reverse_word(left));
            begin += 64;
            end -= 64;
        }
        const std::size_t rest = end - begin;
        if (rest < 2) {
            return;
        }
        // Fewer than 128 bits remain: split them into two halves of at most 64 bits each.
        const auto left_width = static_cast<unsigned>(rest / 2);
        const auto right_width = static_cast<unsigned>(rest - rest / 2);
        const std::uint64_t left = read_unchecked(begin, left_width);
        const std::uint64_t right = read_unchecked(end - right_width, right_width);
        write_unchecked(begin, right_width, reverse_field(right, right_width));
        write_unchecked(begin + right_width, left_width, reverse_field(left, left_width));
    }

    std::size_t bits_;
    std::vector<std::uint64_t> words_;
};
