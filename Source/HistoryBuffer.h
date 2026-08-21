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
    float rawRiderDb = 0.0f;
    float requestedRiderDb = 0.0f;
    float effectiveRiderDb = 0.0f;
    float riderDb = 0.0f;
    float peakEnvelopeDb = -100.0f;
    float peakReductionDb = 0.0f;
    float peakDb = 0.0f;
    float outputDb = -100.0f;
    bool gateActive = false;
};

// Lock-free single-writer, single-reader ring buffer using per-slot sequence numbers.
// Each slot has a sequence counter that the writer increments before and after writing.
// The reader checks that the sequence is even (write complete) before reading.
// This prevents torn reads on ARM/ARM64 where multi-word structs may not be atomic.
class SantosHistoryBuffer
{
public:
    // The release UI only displays the latest 320 points. The former 4096-point
    // buffer existed to support 30/60 second development CSV exports.
    static constexpr std::size_t capacity = 512;

    void clear() noexcept
    {
        for (auto& slot : slots)
            slot.sequence.store(0, std::memory_order_relaxed);
    }

    void push (SantosHistoryPoint point) noexcept
    {
        const auto sequence = writeCount.fetch_add(1, std::memory_order_relaxed);
        const auto index = static_cast<std::size_t>(sequence % capacity);
        auto& slot = slots[index];

        // Increment sequence to odd (write in progress)
        slot.sequence.store(sequence * 2 + 1, std::memory_order_release);
        slot.point = point;
        // Increment sequence to even (write complete)
        slot.sequence.store(sequence * 2 + 2, std::memory_order_release);
    }

    std::vector<SantosHistoryPoint> copyLatest (std::size_t maxPoints) const
    {
        const auto end = writeCount.load(std::memory_order_acquire);
        const auto available = static_cast<std::size_t>(end < capacity ? end : capacity);
        const auto count = std::min(maxPoints, available);

        std::vector<SantosHistoryPoint> result;
        result.reserve(count);

        const auto start = end - static_cast<std::uint64_t>(count);
        for (std::uint64_t seq = start; seq < end; ++seq)
        {
            const auto index = static_cast<std::size_t>(seq % capacity);
            const auto& slot = slots[index];

            // Wait for write to complete (sequence even)
            auto expectedSequence = (seq * 2 + 2);
            while (slot.sequence.load(std::memory_order_acquire) != expectedSequence)
            {
                // Writer is in progress - spin briefly
                // In practice this is extremely fast since audio thread writes once per ~16ms
            }
            result.push_back(slot.point);
        }

        return result;
    }

private:
    struct Slot
    {
        std::atomic<std::uint64_t> sequence { 0 };
        SantosHistoryPoint point {};
    };

    std::array<Slot, capacity> slots {};
    std::atomic<std::uint64_t> writeCount { 0 };
};