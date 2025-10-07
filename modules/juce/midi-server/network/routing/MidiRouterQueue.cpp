/**
 * MidiRouterQueue.cpp
 *
 * Implementation of thread-safe command queue for MidiRouter SEDA architecture.
 */

#include "MidiRouterQueue.h"
#include <chrono>
#include <thread>

namespace NetworkMidi {

void MidiRouterQueue::push(std::unique_ptr<MidiRouterCommands::Command> cmd)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    commandQueue.push_back(std::move(cmd));
}

std::unique_ptr<MidiRouterCommands::Command> MidiRouterQueue::waitAndPop(int timeoutMs)
{
    // WORKAROUND: Use polling instead of condition_variable to avoid macOS libc++ bug
    // macOS libc++ has a bug where condition_variable::wait_for throws
    // "Invalid argument" exception on repeated calls

    auto startTime = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() - startTime < timeout) {
        // Check shutdown flag first
        if (isShutdown.load()) {
            return nullptr;
        }

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

size_t MidiRouterQueue::size() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return commandQueue.size();
}

void MidiRouterQueue::shutdown()
{
    isShutdown.store(true);
}

} // namespace NetworkMidi
