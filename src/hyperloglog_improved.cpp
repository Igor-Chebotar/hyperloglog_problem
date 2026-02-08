#include "hyperloglog_improved.h"

#include <algorithm>
#include <cmath>
#include <numeric>

HyperLogLogImproved::HyperLogLogImproved(size_t b, const HashFuncGen& hasher)
    : b_(b),
      m_(1u << b),
      hasher_(hasher) {
    // m регистров по 6 бит = m*6 бит, округляем вверх до байта
    size_t total_bits = m_ * BITS_PER_REG;
    packed_registers_.resize((total_bits + 7) / 8, 0);
}

void HyperLogLogImproved::SetRegister(size_t idx, uint8_t value) {
    size_t bit_offset = idx * BITS_PER_REG;
    size_t byte_idx = bit_offset / 8;
    size_t bit_idx = bit_offset % 8;

    // Очищаем старое значение и записываем новое
    // может лезть в 2 байта если bit_idx + 6 > 8
    uint16_t mask = ~(static_cast<uint16_t>(0x3F) << bit_idx);
    uint16_t current = static_cast<uint16_t>(packed_registers_[byte_idx]);
    if (byte_idx + 1 < packed_registers_.size()) {
        current |= static_cast<uint16_t>(packed_registers_[byte_idx + 1]) << 8;
    }
    current &= mask;
    current |= static_cast<uint16_t>(value & 0x3F) << bit_idx;

    packed_registers_[byte_idx] = static_cast<uint8_t>(current & 0xFF);
    if (byte_idx + 1 < packed_registers_.size()) {
        packed_registers_[byte_idx + 1] = static_cast<uint8_t>((current >> 8) & 0xFF);
    }
}

uint8_t HyperLogLogImproved::GetRegister(size_t idx) const {
    size_t bit_offset = idx * BITS_PER_REG;
    size_t byte_idx = bit_offset / 8;
    size_t bit_idx = bit_offset % 8;

    uint16_t current = static_cast<uint16_t>(packed_registers_[byte_idx]);
    if (byte_idx + 1 < packed_registers_.size()) {
        current |= static_cast<uint16_t>(packed_registers_[byte_idx + 1]) << 8;
    }
    return static_cast<uint8_t>((current >> bit_idx) & 0x3F);
}

double HyperLogLogImproved::GetAlpha(size_t m) {
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

uint8_t HyperLogLogImproved::CountLeadingZeros(
    uint32_t value, size_t skip_bits) const {
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

void HyperLogLogImproved::Add(const std::string& element) {
    uint32_t hash = hasher_.Hash(element);
    uint32_t idx = hash >> (32 - b_);
    uint8_t rank = CountLeadingZeros(hash, b_);
    uint8_t current = GetRegister(idx);
    if (rank > current) {
        SetRegister(idx, rank);
    }
}

double HyperLogLogImproved::Estimate() const {
    double alpha = GetAlpha(m_);

    // Считаем гармоническое среднее
    double sum = 0.0;
    size_t zero_count = 0;
    for (size_t i = 0; i < m_; ++i) {
        uint8_t reg = GetRegister(i);
        sum += 1.0 / static_cast<double>(1ull << reg);
        if (reg == 0) {
            ++zero_count;
        }
    }

    double raw = alpha * static_cast<double>(m_ * m_) / sum;

    // Коррекция для малых значений (LinearCounting)
    if (raw <= 2.5 * static_cast<double>(m_) && zero_count > 0) {
        double lc = static_cast<double>(m_) *
                    std::log(static_cast<double>(m_) /
                             static_cast<double>(zero_count));
        // плавный переход вместо резкого if-else (из HLL++)
        double weight = raw / (2.5 * static_cast<double>(m_));
        raw = (1.0 - weight) * lc + weight * raw;
    }

    const double TWO32 = 4294967296.0;
    if (raw > TWO32 / 30.0) {
        raw = -TWO32 * std::log(1.0 - raw / TWO32);
    }

    return raw;
}

void HyperLogLogImproved::Reset() {
    std::fill(packed_registers_.begin(), packed_registers_.end(), 0);
}

size_t HyperLogLogImproved::GetMemoryBytes() const {
    return packed_registers_.size();
}
