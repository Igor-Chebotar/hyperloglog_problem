#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

class RandomStreamGen {
public:
    explicit RandomStreamGen(uint64_t seed = 42);

    std::vector<std::string> Generate(size_t stream_size, size_t unique_count);

    std::vector<std::vector<std::string>> SplitByPercent(
        const std::vector<std::string>& stream, size_t step_percent) const;

private:
    std::mt19937_64 rng_;

    std::string GenerateRandomString(size_t max_len);
};
