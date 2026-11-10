#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "08_ols.hpp"
#include "workload.hpp"

struct RunCounts {
    std::uint64_t instructions = 0;
    std::uint64_t calls = 0;
};

struct Sample {
    double instructions = 0;
    double calls = 0;
    double time = 0;
};

struct CostModel {
    double per_instruction = 0;
    double per_call = 0;
};

using Input = std::pair<int, int>;  // (n, base)

RunCounts count_run(int n, int base);

Sample sample_virtual(int n, int base);
CostModel fit_cost_model(std::span<const Sample> samples);
double counts_correlation(std::span<const Sample> samples);

std::vector<Input> design_inputs();

double time_fib_ns(int n, int base, int repeats);
CostModel measure_real_cost(std::span<const Input> inputs, int repeats);
