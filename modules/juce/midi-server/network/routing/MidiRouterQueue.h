/**
 * MidiRouterQueue.h
 *
 * Thread-safe command queue for MidiRouter SEDA architecture.
 * Provides multi-producer, single-consumer command queuing.
 */

#pragma once

#include "MidiRouterCommands.h"
#include <juce_core/juce_core.h>
#include <deque>
#include <memory>
#include <mutex>
#include <atomic>

namespace NetworkMidi {

//==============================================================================
/**
 * Thread-safe command queue for MidiRouter.
 *
 * Design:
 * - Multi-producer safe: Any thread can push commands
 * - Single consumer: Only worker thread should pop
 * - Blocking wait with timeout: Consumer blocks until command available
 * - Unbounded capacity: Uses std::deque (grows as needed)
 *
 * Thread Safety:
 * - push() can be called from any thread
 * - waitAndPop() should only be called from worker thread
 * - size() can be called from any thread
 * - shutdown() can be called to wake up waiting consumer
 */
class MidiRouterQueue {
public:
    MidiRouterQueue() = default;
    ~MidiRouterQueue() = default;

    /**
     * Push command to queue (thread-safe, non-blocking).
     * Called by any thread that needs to send a command to the worker.
     *
     * @param cmd Command to enqueue (ownership transferred)
     */
    void push(std::unique_ptr<MidiRouterCommands::Command> cmd);

    /**
     * Wait for and pop command from queue (blocking with timeout).
     * Should only be called by worker thread.
     *
     * @param timeoutMs Maximum time to wait for command (milliseconds)
     * @return Command or nullptr if timeout occurred or shutdown signaled
     */
    std::unique_ptr<MidiRouterCommands::Command> waitAndPop(int timeoutMs);

    /**
     * Get current queue size (thread-safe).
     *
     * @return Number of commands in queue
     */
    size_t size() const;

    /**
     * Signal shutdown to wake up waiting consumer.
     * After shutdown, waitAndPop() will return nullptr immediately.
     */
    void shutdown();

private:
    mutable std::mutex queueMutex;
    std::deque<std::unique_ptr<MidiRouterCommands::Command>> commandQueue;
    std::atomic<bool> isShutdown{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRouterQueue)
};

} // namespace NetworkMidi
