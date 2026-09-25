// MSVC (C++23)
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <map>
#include <compare>
#include <iomanip>
#include <functional>
#include <deque>
#include <stdexcept>

struct DominoBone {
    int left;
    int right;

    auto operator<=>(const DominoBone&) const = default;
};

class DominoDealer {
private:
    int n;
    std::vector<DominoBone> full_set;
    std::vector<DominoBone> current_pool;
    std::mt19937 rng;

    void build_full_set() {
        full_set.clear();
        for (int i = 0; i <= n; ++i) {
            for (int j = i; j <= n; ++j) {
                full_set.push_back({ i, j });
            }
        }
        std::ranges::sort(full_set);
        auto [first_dup, last] = std::ranges::unique(full_set);
        full_set.erase(first_dup, last);
    }

public:
    explicit DominoDealer(int max_val) : n(max_val) {
        if (max_val < 0) {
            throw std::invalid_argument("Parameter n cannot be negative.");
        }
        std::random_device rd;
        rng.seed(rd());
        build_full_set();
        reset();
    }

    void reset() {
        current_pool = full_set;
        std::ranges::shuffle(current_pool, rng);
    }

    DominoBone operator()() {
        if (current_pool.empty()) {
            throw std::out_of_range("Domino deck is empty.");
        }
        DominoBone bone = current_pool.back();
        current_pool.pop_back();
        return bone;
    }

    [[nodiscard]] int total_bones_count() const noexcept {
        return static_cast<int>(full_set.size());
    }
};

struct DealStats {
    double average;
    double median;
};

DealStats compute_stats(std::vector<int> sizes) {
    if (sizes.empty()) return { 0.0, 0.0 };

    double sum = std::accumulate(sizes.begin(), sizes.end(), 0.0, std::plus<double>());
    double avg = sum / static_cast<double>(sizes.size());

    std::ranges::sort(sizes, std::less<int>());
    size_t sz = sizes.size();
    double med = 0.0;
    if (sz % 2 != 0) {
        med = sizes[sz / 2];
    }
    else {
        med = (sizes[(sz / 2) - 1] + sizes[sz / 2]) / 2.0;
    }

    return { avg, med };
}

int simulate_one_deal(DominoDealer& dealer) {
    dealer.reset();

    DominoBone first;
    try {
        first = dealer();
    }
    catch (const std::out_of_range&) {
        return 0;
    }

    std::deque<DominoBone> chain{ first };
    int deal_size = 1;

    auto attach_left = [&chain](DominoBone b, int end) {
        if (b.right == end) {
            chain.push_front(b);
        }
        else {
            chain.push_front(DominoBone{ b.right, b.left });
        }
        };

    auto attach_right = [&chain](DominoBone b, int end) {
        if (b.left == end) {
            chain.push_back(b);
        }
        else {
            chain.push_back(DominoBone{ b.right, b.left });
        }
        };

    while (true) {
        DominoBone b;
        try {
            b = dealer();
        }
        catch (const std::out_of_range&) {
            break;
        }

        int l_end = chain.front().left;
        int r_end = chain.back().right;

        bool fit_l = (b.left == l_end || b.right == l_end);
        bool fit_r = (b.left == r_end || b.right == r_end);

        if (!fit_l && !fit_r) break;

        if (fit_l && fit_r) {
            if (l_end <= r_end) {
                attach_left(b, l_end);
            }
            else {
                attach_right(b, r_end);
            }
        }
        else if (fit_l) {
            attach_left(b, l_end);
        }
        else {
            attach_right(b, r_end);
        }
        ++deal_size;
    }
    return deal_size;
}

int main() {
    try {
        int n = 0;
        int total_deals = 0;

        std::cout << "Enter n (max bone value): ";
        if (!(std::cin >> n)) {
            throw std::runtime_error("Invalid input for parameter n.");
        }
        std::cout << "Enter number of deals: ";
        if (!(std::cin >> total_deals) || total_deals <= 0) {
            throw std::runtime_error("Deals count must be a positive integer.");
        }
        DominoDealer dealer(n);
        int total_bones = dealer.total_bones_count();

        std::vector<int> deal_sizes;
        deal_sizes.reserve(total_deals);
        std::map<int, int> freq;
        for (int i = 0; i < total_deals; ++i) {
            int sz = simulate_one_deal(dealer);
            deal_sizes.push_back(sz);
            freq[sz]++;
        }
        std::cout << "\nResults (n = " << n << ", Total bones = " << total_bones << ", Deals = " << total_deals << ")\n";
        std::cout << "Size Distribution (all potential sizes 1.." << total_bones << "):\n";

        auto print_dist = [total_deals, &freq](int sz) {
            int count = 0;
            if (freq.contains(sz)) {
                count = freq.at(sz);
            }
            double pct = (static_cast<double>(count) / total_deals) * 100.0;
            std::cout << "  Size " << std::setw(2) << sz << ": "
                << std::fixed << std::setprecision(1) << std::setw(5) << pct
                << "% (" << count << ")\n";
            };
        for (int sz = 1; sz <= total_bones; ++sz) {
            print_dist(sz);
        }
        auto mode_it = std::ranges::max_element(freq, [](const auto& a, const auto& b) {
            return a.second < b.second;
            });

        auto [avg, median] = compute_stats(deal_sizes);

        std::cout << "\nStatistics:\n";
        std::cout << "  Most frequent size (Mode): " << mode_it->first<< " (" << mode_it->second << " times; first on tie)\n";
        std::cout << "  Average size: " << std::fixed << std::setprecision(2) << avg << "\n";
        std::cout << "  Median size:  " << std::fixed << std::setprecision(2) << median << "\n";

        std::cout << "\nExperiment (Ratios for different n)\n";
        std::cout << "  n | Bones | Avg Size | Avg/Total | Med/Total\n";
        std::cout << "----+-------+----------+-----------+----------\n";

        constexpr int EXPERIMENT_MAX_N = 7;
        constexpr int EXPERIMENT_RUNS = 500;
        for (int test_n = 1; test_n <= EXPERIMENT_MAX_N; ++test_n) {
            DominoDealer exp_dealer(test_n);
            int exp_total = exp_dealer.total_bones_count();
            std::vector<int> exp_sizes;
            exp_sizes.reserve(EXPERIMENT_RUNS);

            for (int r = 0; r < EXPERIMENT_RUNS; ++r) {
                exp_sizes.push_back(simulate_one_deal(exp_dealer));
            }
            auto [e_avg, e_med] = compute_stats(exp_sizes);
            std::cout << std::setw(3) << test_n << " | "
                << std::setw(5) << exp_total << " | "
                << std::setw(8) << std::fixed << std::setprecision(2) << e_avg << " | "
                << std::setw(9) << std::fixed << std::setprecision(4) << (e_avg / exp_total) << " | "
                << std::setw(9) << std::fixed << std::setprecision(4) << (e_med / exp_total) << "\n";
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}