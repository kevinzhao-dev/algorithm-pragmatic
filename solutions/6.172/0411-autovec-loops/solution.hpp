#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

// Stage 1: a plain loop; clang emits a runtime overlap check and two versions of the loop.
[[gnu::noinline]] inline void add_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = static_cast<std::uint8_t>(a[i] + b[i]);
    }
}

// Stage 2: a select writes every element, so no conditional store is needed.
[[gnu::noinline]] inline void max_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = b[i] > a[i] ? b[i] : a[i];
    }
}

// Stage 3: reassociation lets the reduction run lane-wise and be summed at the end.
[[gnu::noinline]] inline double sum_doubles(const double* x, std::size_t n) {
#pragma clang fp reassociate(on)
    double total = 0;
    for (std::size_t i = 0; i < n; ++i) {
        total += x[i];
    }
    return total;
}

[[gnu::noinline]] inline float dot_floats(const float* a, const float* b, std::size_t n) {
#pragma clang fp reassociate(on)
    float total = 0;
    for (std::size_t i = 0; i < n; ++i) {
        total += a[i] * b[i];
    }
    return total;
}

// Stage 4
template <typename T>
[[gnu::noinline]] void add_lanes(T* out, const T* a, const T* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = static_cast<T>(a[i] + b[i]);
    }
}

// Stage 5
class ParticleStore {
public:
    explicit ParticleStore(std::size_t count)
        : x_(count), y_(count), z_(count), vx_(count), vy_(count), vz_(count) {}

    std::size_t size() const { return x_.size(); }
    std::span<float> x() { return x_; }
    std::span<float> y() { return y_; }
    std::span<float> z() { return z_; }
    std::span<float> vx() { return vx_; }
    std::span<float> vy() { return vy_; }
    std::span<float> vz() { return vz_; }

    [[gnu::noinline]] void integrate(float dt) {
        advance(x_.data(), vx_.data(), dt);
        advance(y_.data(), vy_.data(), dt);
        advance(z_.data(), vz_.data(), dt);
    }

private:
    void advance(float* position, const float* velocity, float dt) {
        const std::size_t n = size();
        for (std::size_t i = 0; i < n; ++i) {
            position[i] += velocity[i] * dt;
        }
    }

    std::vector<float> x_, y_, z_, vx_, vy_, vz_;
};

// Stage 6: the accumulator lives in a register, not in out[i].
[[gnu::noinline]] inline void row_sums(std::span<const int> matrix, std::size_t rows,
                                       std::size_t cols, std::span<int> out) {
    for (std::size_t row = 0; row < rows; ++row) {
        const int* const cells = matrix.data() + row * cols;
        int total = 0;
        for (std::size_t col = 0; col < cols; ++col) {
            total += cells[col];
        }
        out[row] = total;
    }
}

// Stage 7
[[gnu::noinline]] inline std::optional<std::size_t> best_match(std::span<const float> query,
                                                               std::span<const float> matrix,
                                                               std::size_t dim) {
    if (dim == 0 || query.size() != dim || matrix.size() % dim != 0) {
        throw std::invalid_argument("bad matrix shape");
    }
    const std::size_t rows = matrix.size() / dim;
    if (rows == 0) {
        return std::nullopt;
    }
    std::size_t best_row = 0;
    float best_score = dot_floats(query.data(), matrix.data(), dim);
    for (std::size_t row = 1; row < rows; ++row) {
        const float score = dot_floats(query.data(), matrix.data() + row * dim, dim);
        if (score > best_score) {
            best_score = score;
            best_row = row;
        }
    }
    return best_row;
}
