#include "RealtimeMidiBuffer.h"

namespace NetworkMidi {

bool RealtimeMidiBuffer::write(const MidiPacket& packet) {
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 == 0) {
        // Buffer full - implement drop-oldest policy
        droppedCount.fetch_add(1, std::memory_order_relaxed);

        // Force-advance read pointer to make space
        // This drops the oldest message
        int readStart, readSize, dummy1, dummy2;
        fifo.prepareToRead(1, readStart, readSize, dummy1, dummy2);
        if (readSize > 0) {
            fifo.finishedRead(1);  // Discard oldest
        }

        // Try again
        fifo.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 == 0) {
            return false;  // Still can't write (shouldn't happen)
        }
    }

    buffer[start1] = packet;
    fifo.finishedWrite(1);
    totalWritten.fetch_add(1, std::memory_order_relaxed);
    return true;
}

int RealtimeMidiBuffer::readBatch(MidiPacket* dest, int maxCount) {
    int start1, size1, start2, size2;
    fifo.prepareToRead(maxCount, start1, size1, start2, size2);

    int totalReadCount = 0;

    // Copy first contiguous block
    for (int i = 0; i < size1; ++i) {
        dest[totalReadCount++] = buffer[start1 + i];
    }

    // Copy second block if ring wraps around
    for (int i = 0; i < size2; ++i) {
        dest[totalReadCount++] = buffer[start2 + i];
    }

    if (totalReadCount > 0) {
        fifo.finishedRead(totalReadCount);
        totalRead.fetch_add(totalReadCount, std::memory_order_relaxed);
    }

    return totalReadCount;
}

RealtimeMidiBuffer::Stats RealtimeMidiBuffer::getStats() const {
    Stats s;
    s.numReady = fifo.getNumReady();
    s.freeSpace = fifo.getFreeSpace();
    s.dropped = droppedCount.load(std::memory_order_relaxed);
    s.written = totalWritten.load(std::memory_order_relaxed);
    s.read = totalRead.load(std::memory_order_relaxed);
    s.dropRate = (s.written > 0) ? (100.0f * s.dropped / s.written) : 0.0f;
    return s;
}

} // namespace NetworkMidi
