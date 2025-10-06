/**
 * NetworkConnectionQueue.cpp
 *
 * Implementation of thread-safe command queue for SEDA architecture.
 */

#include "NetworkConnectionQueue.h"

namespace NetworkMidi {

void NetworkConnectionQueue::pushCommand(std::unique_ptr<Commands::Command> cmd)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        commandQueue.push_back(std::move(cmd));
    }

    // Signal event AFTER releasing lock to avoid waking worker while we hold lock
    queueEvent.signal();
}

std::unique_ptr<Commands::Command> NetworkConnectionQueue::waitAndPop(int timeoutMs)
{
    // Wait for signal with timeout
    if (!queueEvent.wait(timeoutMs)) {
        return nullptr;  // Timeout - no command available
    }

    std::lock_guard<std::mutex> lock(queueMutex);

    if (commandQueue.empty()) {
        return nullptr;  // Spurious wakeup - no command
    }

    // Move command out of queue
    auto cmd = std::move(commandQueue.front());
    commandQueue.pop_front();

    // If more commands remain, re-signal event
    if (!commandQueue.empty()) {
        queueEvent.signal();
    }

    return cmd;
}

size_t NetworkConnectionQueue::size() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return commandQueue.size();
}

} // namespace NetworkMidi
