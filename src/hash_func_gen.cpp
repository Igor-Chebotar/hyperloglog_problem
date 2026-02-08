#include "hash_func_gen.h"

HashFuncGen::HashFuncGen(uint64_t seed) : rng_(seed) {
    GenerateCoefficients();
}

void HashFuncGen::GenerateCoefficients() {
    std::uniform_int_distribution<uint64_t> dist(1, UINT32_MAX);
    coeff_a_ = dist(rng_);
    coeff_b_ = dist(rng_);
}

void HashFuncGen::Regenerate() {
    GenerateCoefficients();
}

uint32_t HashFuncGen::Hash(const std::string& key) const {
    // poly hash + linear transform + murmur finalization
    uint64_t poly = 0;
    const uint64_t BASE = 257;
    for (const char& c : key) {
        poly = poly * BASE + static_cast<uint8_t>(c);
        poly &= 0xFFFFFFFF;
    }
    uint32_t result = static_cast<uint32_t>((coeff_a_ * poly + coeff_b_) & 0xFFFFFFFF);
    // Дополнительное перемешивание по типу финализации murmur
    result ^= result >> 16;
    result *= 0x85ebca6b;
    result ^= result >> 13;
    result *= 0xc2b2ae35;
    result ^= result >> 16;
    return result;
}
