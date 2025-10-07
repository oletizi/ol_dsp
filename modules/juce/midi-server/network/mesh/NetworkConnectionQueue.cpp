/**
 * NetworkConnectionQueue.cpp
 *
 * Implementation of thread-safe command queue for SEDA architecture.
 */

#include "NetworkConnectionQueue.h"
#include <chrono>

namespace NetworkMidi {

void NetworkConnectionQueue::pushCommand(std::unique_ptr<Commands::Command> cmd)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        commandQueue.push_back(std::move(cmd));
    }

    // Notify waiting thread
    queueCondition.notify_one();
}

std::unique_ptr<Commands::Command> NetworkConnectionQueue::waitAndPop(int timeoutMs)
{
    // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
    // macOS libc++ has a bug where condition_variable::wait_for throws
    // "Invalid argument" exception on repeated calls

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() - startTime < timeout) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!commandQueue.empty()) {
                auto cmd = std::move(commandQueue.front());
                commandQueue.pop_front();
                return cmd;
            }
        }

        // Sleep for 1ms to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return nullptr;  // Timeout
}

size_t NetworkConnectionQueue::size() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return commandQueue.size();
}

} // namespace NetworkMidi
