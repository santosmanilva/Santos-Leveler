#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

struct SantosHistoryPoint
{
    float inputDb = -100.0f;
    float fastDb = -100.0f;
    float slowDb = -100.0f;
    float controlDb = -100.0f;
    float requestedRiderDb = 0.0f;
    float effectiveRiderDb = 0.0f;
    float riderDb = 0.0f;
    float peakEnvelopeDb = -100.0f;
    float peakReductionDb = 0.0f;
    float peakDb = 0.0f;
    float outputDb = -100.0f;
    bool gateActive = false;
};

class SantosHistoryBuffer
{
public:
    static constexpr std::size_t capacity = 4096;

    void clear() noexcept
    {
        writeCount.store (0, std::memory_order_release);
    }

    void push (SantosHistoryPoint point) noexcept
    {
        const auto sequence = writeCount.load (std::memory_order_relaxed);
        points[static_cast<std::size_t> (sequence % capacity)] = point;
        writeCount.store (sequence + 1, std::memory_order_release);
    }

    std::vector<SantosHistoryPoint> copyLatest (std::size_t maxPoints) const
    {
        const auto end = writeCount.load (std::memory_order_acquire);
        const auto available = static_cast<std::size_t> (end < capacity ? end : capacity);
        const auto count = std::min (maxPoints, available);

        std::vector<SantosHistoryPoint> result;
        result.reserve (count);

        const auto start = end - static_cast<std::uint64_t> (count);
        for (std::uint64_t seq = start; seq < end; ++seq)
            result.push_back (points[static_cast<std::size_t> (seq % capacity)]);

        return result;
    }

private:
    std::array<SantosHistoryPoint, capacity> points {};
    std::atomic<std::uint64_t> writeCount { 0 };
};
