#include "random_stream_gen.h"

#include <algorithm>

static const std::string CHARSET =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-";

RandomStreamGen::RandomStreamGen(uint64_t seed) : rng_(seed) {}

std::string RandomStreamGen::GenerateRandomString(size_t max_len) {
    std::uniform_int_distribution<size_t> len_dist(1, max_len);
    size_t len = len_dist(rng_);

    std::uniform_int_distribution<size_t> char_dist(0, CHARSET.size() - 1);
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        result += CHARSET[char_dist(rng_)];
    }
    return result;
}

std::vector<std::string> RandomStreamGen::Generate(
    size_t stream_size, size_t unique_count) {
    std::vector<std::string> unique_strings;
    unique_strings.reserve(unique_count);
    for (size_t i = 0; i < unique_count; ++i) {
        unique_strings.push_back(GenerateRandomString(30));
    }

    std::uniform_int_distribution<size_t> pick_dist(0, unique_count - 1);
    std::vector<std::string> stream;
    stream.reserve(stream_size);
    for (size_t i = 0; i < stream_size; ++i) {
        stream.push_back(unique_strings[pick_dist(rng_)]);
    }
    return stream;
}

std::vector<std::vector<std::string>> RandomStreamGen::SplitByPercent(
    const std::vector<std::string>& stream, size_t step_percent) const {
    std::vector<std::vector<std::string>> parts;
    for (size_t pct = step_percent; pct <= 100; pct += step_percent) {
        size_t end_idx = stream.size() * pct / 100;
        parts.emplace_back(stream.begin(), stream.begin() + end_idx);
    }
    return parts;
}
