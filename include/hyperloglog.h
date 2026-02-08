#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hash_func_gen.h"

class HyperLogLog {
public:
    explicit HyperLogLog(size_t b, const HashFuncGen& hasher);

    void Add(const std::string& element);
    double Estimate() const;
    void Reset();

    size_t GetB() const { return b_; }
    size_t GetM() const { return m_; }

private:
    size_t b_;
    size_t m_;
    std::vector<uint8_t> registers_;
    const HashFuncGen& hasher_;

    static double GetAlpha(size_t m);
    uint8_t CountLeadingZeros(uint32_t value, size_t skip_bits) const;
};
