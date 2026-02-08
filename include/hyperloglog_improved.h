#pragma once

#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

#include "hash_func_gen.h"

// Версия с компактным хранением регистров (6 бит вместо 8)
// и плавной коррекцией малых значений из HLL++

class HyperLogLogImproved {
public:
    explicit HyperLogLogImproved(size_t b, const HashFuncGen& hasher);

    void Add(const std::string& element);
    double Estimate() const;
    void Reset();

    size_t GetB() const { return b_; }
    size_t GetM() const { return m_; }
    size_t GetMemoryBytes() const;

private:
    size_t b_;
    size_t m_;
    const HashFuncGen& hasher_;

    // 6 бит хватает, т.к. значение регистра <= 32-B
    static const size_t BITS_PER_REG = 6;
    std::vector<uint8_t> packed_registers_;

    void SetRegister(size_t idx, uint8_t value);
    uint8_t GetRegister(size_t idx) const;

    static double GetAlpha(size_t m);
    uint8_t CountLeadingZeros(uint32_t value, size_t skip_bits) const;
};
