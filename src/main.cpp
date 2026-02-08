#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "exact_counter.h"
#include "hash_func_gen.h"
#include "hyperloglog.h"
#include "hyperloglog_improved.h"
#include "random_stream_gen.h"

struct StreamConfig {
    size_t stream_size;
    size_t unique_count;
};

struct StepResult {
    size_t step_number;
    double percent;
    size_t exact;
    double estimate;
    double estimate_improved;
};

void RunSingleStream(
    const std::vector<std::string>& stream,
    size_t step_percent,
    size_t b,
    const HashFuncGen& hasher,
    std::vector<StepResult>& results) {
    HyperLogLog hll(b, hasher);
    HyperLogLogImproved hll_imp(b, hasher);

    size_t step_size = stream.size() * step_percent / 100;
    size_t step_num = 0;

    for (size_t pct = step_percent; pct <= 100; pct += step_percent) {
        size_t start = step_num * step_size;
        size_t end = stream.size() * pct / 100;

        for (size_t i = start; i < end; ++i) {
            hll.Add(stream[i]);
            hll_imp.Add(stream[i]);
        }

        std::vector<std::string> prefix(stream.begin(),
                                        stream.begin() + end);
        size_t exact = ExactCounter::Count(prefix);

        StepResult r;
        r.step_number = step_num + 1;
        r.percent = static_cast<double>(pct);
        r.exact = exact;
        r.estimate = hll.Estimate();
        r.estimate_improved = hll_imp.Estimate();
        results.push_back(r);

        ++step_num;
    }
}

void WriteCsvSingleRun(const std::string& filename,
                       const std::vector<StepResult>& results) {
    std::ofstream out(filename);
    out << "step,percent,exact,estimate_hll,estimate_improved\n";
    for (const auto& r : results) {
        out << r.step_number << ","
            << r.percent << ","
            << r.exact << ","
            << std::fixed << std::setprecision(1) << r.estimate << ","
            << r.estimate_improved << "\n";
    }
    out.close();
}

struct AggregatedStep {
    size_t step_number;
    double percent;
    double mean_exact;
    double mean_estimate;
    double std_estimate;
    double mean_estimate_imp;
    double std_estimate_imp;
};

std::vector<AggregatedStep> AggregateRuns(
    const std::vector<std::vector<StepResult>>& all_runs) {
    if (all_runs.empty()) return {};
    size_t num_steps = all_runs[0].size();
    size_t num_runs = all_runs.size();

    std::vector<AggregatedStep> agg(num_steps);

    for (size_t s = 0; s < num_steps; ++s) {
        agg[s].step_number = all_runs[0][s].step_number;
        agg[s].percent = all_runs[0][s].percent;

        double sum_exact = 0, sum_est = 0, sum_imp = 0;
        for (size_t r = 0; r < num_runs; ++r) {
            sum_exact += static_cast<double>(all_runs[r][s].exact);
            sum_est += all_runs[r][s].estimate;
            sum_imp += all_runs[r][s].estimate_improved;
        }
        agg[s].mean_exact = sum_exact / static_cast<double>(num_runs);
        agg[s].mean_estimate = sum_est / static_cast<double>(num_runs);
        agg[s].mean_estimate_imp = sum_imp / static_cast<double>(num_runs);

        double var_est = 0, var_imp = 0;
        for (size_t r = 0; r < num_runs; ++r) {
            double diff = all_runs[r][s].estimate - agg[s].mean_estimate;
            var_est += diff * diff;
            double diff2 = all_runs[r][s].estimate_improved - agg[s].mean_estimate_imp;
            var_imp += diff2 * diff2;
        }
        agg[s].std_estimate = std::sqrt(var_est / static_cast<double>(num_runs));
        agg[s].std_estimate_imp = std::sqrt(var_imp / static_cast<double>(num_runs));
    }
    return agg;
}

void WriteCsvAggregated(const std::string& filename,
                        const std::vector<AggregatedStep>& agg) {
    std::ofstream out(filename);
    out << "step,percent,mean_exact,mean_hll,std_hll,mean_improved,std_improved\n";
    for (const auto& a : agg) {
        out << a.step_number << ","
            << a.percent << ","
            << std::fixed << std::setprecision(1)
            << a.mean_exact << ","
            << a.mean_estimate << ","
            << a.std_estimate << ","
            << a.mean_estimate_imp << ","
            << a.std_estimate_imp << "\n";
    }
    out.close();
}

void TestBValues(const std::vector<std::string>& stream,
                 const HashFuncGen& hasher) {
    std::ofstream out("data/b_test.csv");
    out << "b,m,exact,estimate,error_pct\n";

    size_t exact = ExactCounter::Count(stream);

    for (size_t b = 4; b <= 16; ++b) {
        HyperLogLog hll(b, hasher);
        for (const auto& s : stream) {
            hll.Add(s);
        }
        double est = hll.Estimate();
        double err = std::abs(est - static_cast<double>(exact)) /
                     static_cast<double>(exact) * 100.0;
        out << b << "," << (1u << b) << "," << exact << ","
            << std::fixed << std::setprecision(1) << est << ","
            << std::setprecision(2) << err << "\n";

        std::cout << "  B=" << b << " (m=" << (1u << b)
                  << "): estimate=" << std::fixed << std::setprecision(0)
                  << est << ", exact=" << exact
                  << ", error=" << std::setprecision(2) << err << "%\n";
    }
    out.close();
}

void TestMemoryUsage(size_t b) {
    std::ofstream out("data/memory.csv");
    out << "version,b,m,bytes\n";

    size_t m = 1u << b;
    size_t std_bytes = m * sizeof(uint8_t);  // по байту на регистр
    size_t imp_bytes = (m * 6 + 7) / 8;  // 6 бит на регистр

    out << "standard," << b << "," << m << "," << std_bytes << "\n";
    out << "improved," << b << "," << m << "," << imp_bytes << "\n";
    out.close();

    std::cout << "\nMemory comparison (B=" << b << ", m=" << m << "):\n";
    std::cout << "  Standard: " << std_bytes << " bytes\n";
    std::cout << "  Improved: " << imp_bytes << " bytes\n";
    std::cout << "  Saving: "
              << std::fixed << std::setprecision(1)
              << (1.0 - static_cast<double>(imp_bytes) /
                  static_cast<double>(std_bytes)) * 100.0
              << "%\n";
}

int main() {
    const size_t CHOSEN_B = 10;
    const size_t STEP_PERCENT = 5;
    const size_t NUM_STREAMS = 10;

    std::vector<StreamConfig> configs = {
        {100'000, 10'000},
        {500'000, 50'000},
        {1'000'000, 100'000},
    };

    HashFuncGen hasher(777);

    // --- Тест значений B ---
    std::cout << "=== Testing B values ===\n";
    {
        RandomStreamGen gen(100);
        auto test_stream = gen.Generate(200'000, 20'000);
        TestBValues(test_stream, hasher);
    }

    // --- Основные эксперименты ---
    for (size_t cfg_idx = 0; cfg_idx < configs.size(); ++cfg_idx) {
        const auto& cfg = configs[cfg_idx];
        std::cout << "\n=== Config " << cfg_idx + 1
                  << ": size=" << cfg.stream_size
                  << ", unique=" << cfg.unique_count << " ===\n";

        std::vector<std::vector<StepResult>> all_runs;

        for (size_t run = 0; run < NUM_STREAMS; ++run) {
            RandomStreamGen gen(run * 1000 + cfg_idx * 100);
            auto stream = gen.Generate(cfg.stream_size, cfg.unique_count);

            std::vector<StepResult> results;
            RunSingleStream(stream, STEP_PERCENT, CHOSEN_B, hasher, results);
            all_runs.push_back(results);

            if (run == 0) {
                std::string fname = "data/single_run_cfg" +
                                    std::to_string(cfg_idx + 1) + ".csv";
                WriteCsvSingleRun(fname, results);
            }

            std::cout << "  Run " << run + 1 << "/" << NUM_STREAMS
                      << " done\n";
        }

        auto agg = AggregateRuns(all_runs);
        std::string agg_fname = "data/aggregated_cfg" +
                                std::to_string(cfg_idx + 1) + ".csv";
        WriteCsvAggregated(agg_fname, agg);

        // Вывод итоговых ошибок
        if (!agg.empty()) {
            const auto& last = agg.back();
            double rel_err = std::abs(last.mean_estimate - last.mean_exact) /
                             last.mean_exact * 100.0;
            double theory_1 = 1.04 / std::sqrt(static_cast<double>(1u << CHOSEN_B));
            double theory_2 = 1.3 / std::sqrt(static_cast<double>(1u << CHOSEN_B));
            std::cout << "  Final: mean_exact=" << std::fixed
                      << std::setprecision(0) << last.mean_exact
                      << ", mean_est=" << last.mean_estimate
                      << ", relative_err=" << std::setprecision(2)
                      << rel_err << "%"
                      << ", theory_bound(1.04/sqrt(m))="
                      << std::setprecision(2) << theory_1 * 100.0 << "%"
                      << ", theory_bound(1.3/sqrt(m))="
                      << std::setprecision(2) << theory_2 * 100.0 << "%"
                      << "\n";
        }
    }

    // --- Сравнение памяти ---
    TestMemoryUsage(CHOSEN_B);

    std::cout << "\nAll data written to data/ directory.\n";
    std::cout << "Run 'python3 scripts/plot.py' to generate plots.\n";

    return 0;
}
