#pragma once

#include <cstdint>
#include <random>
#include <string>

class HashFuncGen {
public:
    explicit HashFuncGen(uint64_t seed = 12345);

    uint32_t Hash(const std::string& key) const;

    void Regenerate();

private:
    std::mt19937_64 rng_;
    uint64_t coeff_a_;
    uint64_t coeff_b_;

    void GenerateCoefficients();
};
