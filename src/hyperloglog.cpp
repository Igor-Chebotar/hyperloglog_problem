#include "hyperloglog.h"

#include <algorithm>
#include <cmath>

HyperLogLog::HyperLogLog(size_t b, const HashFuncGen& hasher)
    : b_(b),
      m_(1u << b),
      registers_(1u << b, 0),
      hasher_(hasher) {}

double HyperLogLog::GetAlpha(size_t m) {
    switch (m) {
        case 16:
            return 0.673;
        case 32:
            return 0.697;
        case 64:
            return 0.709;
        default:
            return 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
    }
}

uint8_t HyperLogLog::CountLeadingZeros(uint32_t value, size_t skip_bits) const {
    // rank = позиция первой единицы в оставшихся битах
    uint32_t remaining = value << skip_bits;
    size_t bits_left = 32 - skip_bits;
    uint8_t count = 1;
    for (size_t i = 0; i < bits_left; ++i) {
        if ((remaining & 0x80000000) == 0) {
            ++count;
            remaining <<= 1;
        } else {
            break;
        }
    }
    return count;
}

void HyperLogLog::Add(const std::string& element) {
    uint32_t hash = hasher_.Hash(element);
    uint32_t idx = hash >> (32 - b_);
    uint8_t rank = CountLeadingZeros(hash, b_);
    if (rank > registers_[idx]) {
        registers_[idx] = rank;
    }
}

double HyperLogLog::Estimate() const {
    double alpha = GetAlpha(m_);
    double sum = 0.0;
    for (size_t i = 0; i < m_; ++i) {
        sum += 1.0 / static_cast<double>(1ull << registers_[i]);
    }
    double raw_estimate = alpha * static_cast<double>(m_) *
                          static_cast<double>(m_) / sum;

    // small range correction (linear counting)
    if (raw_estimate <= 2.5 * static_cast<double>(m_)) {
        size_t zeros = 0;
        for (size_t i = 0; i < m_; ++i) {
            if (registers_[i] == 0) {
                ++zeros;
            }
        }
        if (zeros > 0) {
            raw_estimate = static_cast<double>(m_) *
                           std::log(static_cast<double>(m_) /
                                    static_cast<double>(zeros));
        }
    }

    // large range correction
    const double TWO_POW_32 = 4294967296.0;
    if (raw_estimate > TWO_POW_32 / 30.0) {
        raw_estimate = -TWO_POW_32 *
                       std::log(1.0 - raw_estimate / TWO_POW_32);
    }

    return raw_estimate;
}

void HyperLogLog::Reset() {
    std::fill(registers_.begin(), registers_.end(), 0);
}
