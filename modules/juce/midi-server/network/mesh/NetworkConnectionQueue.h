/**
 * NetworkConnectionQueue.h
 *
 * Thread-safe command queue for SEDA architecture.
 * Provides multi-producer, single-consumer command queuing with blocking wait.
 */

#pragma once

#include "Commands.h"
#include <juce_events/juce_events.h>
#include <deque>
#include <memory>
#include <mutex>

namespace NetworkMidi {

//==============================================================================
/**
 * Thread-safe command queue using juce::WaitableEvent for blocking operations.
 *
 * Design:
 * - Multi-producer safe: Any thread can push commands
 * - Single consumer: Only worker thread should pop
 * - Blocking wait: Consumer blocks until command available
 * - Unbounded capacity: Uses std::deque (grows as needed)
 *
 * Thread Safety:
 * - pushCommand() can be called from any thread
 * - waitAndPop() should only be called from worker thread
 * - size() can be called from any thread
 */
class NetworkConnectionQueue {
public:
    NetworkConnectionQueue() = default;
    ~NetworkConnectionQueue() = default;

    /**
     * Push command to queue (thread-safe, non-blocking).
     * Called by any thread that needs to send a command to the worker.
     *
     * @param cmd Command to enqueue (ownership transferred)
     */
    void pushCommand(std::unique_ptr<Commands::Command> cmd);

    /**
     * Wait for and pop command from queue (blocking).
     * Should only be called by worker thread.
     *
     * @param timeoutMs Maximum time to wait for command (milliseconds)
     * @return Command or nullptr if timeout occurred
     */
    std::unique_ptr<Commands::Command> waitAndPop(int timeoutMs);

    /**
     * Get current queue size (thread-safe).
     *
     * @return Number of commands in queue
     */
    size_t size() const;

private:
    mutable std::mutex queueMutex;
    std::deque<std::unique_ptr<Commands::Command>> commandQueue;
    juce::WaitableEvent queueEvent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkConnectionQueue)
};

} // namespace NetworkMidi
