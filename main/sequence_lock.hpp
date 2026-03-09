#pragma once

#ifndef DEVCONTAINER_JSON_SEQUENCE_LOCK_HPP
#define DEVCONTAINER_JSON_SEQUENCE_LOCK_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>


/** Non-blocking writer, with blocking readier.
 *  Thread safe.
 *  @tparam T Type of data being stored.
 *
 *  @see https://en.wikipedia.org/wiki/Seqlock
 */
template <typename T>
class SequenceLock {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

private:
    std::atomic<uint32_t> _seq{0};
protected:
    T _data;

protected:
    template <typename Data>
    void write(Data const& newData, std::function<void(Data const&)> setter ) {
        // Writer: Increments to odd, writes, increments to even

        uint32_t seq = _seq.load(std::memory_order_relaxed);
        _seq.store(seq + 1, std::memory_order_release); // Start to write (now odd)

        std::atomic_signal_fence(std::memory_order_acq_rel);
        setter(newData);
        std::atomic_signal_fence(std::memory_order_acq_rel);

        _seq.store(seq + 2, std::memory_order_release); // End write (now even)
    }

    std::optional<T> try_read() const
    {
        T temp;

        uint32_t seq1 = _seq.load(std::memory_order_acquire);
        // If seq is odd, a writer is currently active. Wait/Yield.
        if (seq1 & 1) {
            return std::nullopt;
        }

        temp = _data;

        std::atomic_signal_fence(std::memory_order_acquire);

        uint32_t seq2 = _seq.load(std::memory_order_acquire);
        if (seq1 != seq2) {
            // A writer updated the data while we re trying to read.
            return std::nullopt;
        }
        return _data;
    }

public:
    /**
     * @returns a copy of the data.
     */
    T read() const {
        // Reader: Loops until a clean "snapshot" is taken
        while (true)
        {
            std::optional<T> temp = try_read();
            if (temp.has_value()) {
                return temp.value();
            }
        }
    }
};

#endif 
