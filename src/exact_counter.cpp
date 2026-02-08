#include "exact_counter.h"

size_t ExactCounter::Count(const std::vector<std::string>& stream) {
    std::unordered_set<std::string> seen;
    for (const auto& s : stream) {
        seen.insert(s);
    }
    return seen.size();
}
