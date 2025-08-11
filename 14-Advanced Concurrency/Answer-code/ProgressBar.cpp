#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <print>
#include <string>

class ProgressBar
{
    static inline constexpr char s_emptyChar = ' ';
    static inline constexpr char s_fillChar = '#';
    static inline constexpr char s_barLength = 50;

    std::array<char, s_barLength> outputBuffer_;

    std::size_t maxCnt_;
    std::size_t lastUpdatedCnt_ = 0;
    std::atomic<bool> barGuard_{ false };
    std::atomic<std::size_t> cnt_{ 0 };

public:
    ProgressBar(std::size_t maxCnt) : maxCnt_{ maxCnt }
    {
        outputBuffer_.fill(s_emptyChar);
        std::println();
    }

    void Update(std::size_t inc = 1)
    {
        auto lastCnt = cnt_.fetch_add(inc, std::memory_order_acquire);
        auto currCnt = lastCnt + inc;

        bool reachEnd = currCnt >= maxCnt_,
             previousReachEnd = lastCnt >= maxCnt_;
        if (previousReachEnd)
            return;

        auto restoreGuard = [this]() {
            barGuard_.store(false, std::memory_order_release);
            barGuard_.notify_one();
        };

        while (true)
        {
            bool expected = false;
            if (barGuard_.compare_exchange_strong(expected, true,
                                                  std::memory_order_acquire))
            {
                if (currCnt < lastUpdatedCnt_)
                {
                    restoreGuard();
                    break;
                }

                double currRatio =
                    std::min(static_cast<double>(currCnt) / maxCnt_, 1.0);
                auto fillLen = static_cast<std::size_t>(
                    std::round(currRatio * s_barLength));
                // There should be no overflow caused by rounding error.
                assert(fillLen <= s_barLength);
                // When maximum length is too large, we can also fill only in
                // [lastUpdated, curr].
                std::ranges::fill_n(std::begin(outputBuffer_), fillLen,
                                    s_fillChar);

                // Since C++23
                std::string_view bufferView{ outputBuffer_ };
                std::print("\r[{}] {:.2f}%", bufferView, currRatio * 100);
                lastUpdatedCnt_ = currCnt;
                restoreGuard();
                break;
            }

            if (!reachEnd) [[likely]]
                break;
            barGuard_.wait(true, std::memory_order_relaxed);
        }
        return;
    }
};