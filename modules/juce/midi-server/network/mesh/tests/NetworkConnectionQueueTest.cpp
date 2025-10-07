/**
 * NetworkConnectionQueueTest.cpp
 *
 * Unit tests for NetworkConnectionQueue (SEDA infrastructure).
 */

#include "../NetworkConnectionQueue.h"
#include "../Commands.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace NetworkMidi;

//==============================================================================
// Basic functionality tests

TEST(NetworkConnectionQueueTest, PushAndPop) {
    NetworkConnectionQueue queue;

    // Push a command
    queue.pushCommand(std::make_unique<Commands::ConnectCommand>());

    // Verify size
    EXPECT_EQ(queue.size(), 1u);

    // Pop command
    auto cmd = queue.waitAndPop(100);
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->type, Commands::Command::Connect);

    // Queue should be empty
    EXPECT_EQ(queue.size(), 0u);
}

TEST(NetworkConnectionQueueTest, MultipleCommands) {
    NetworkConnectionQueue queue;

    // Push multiple commands
    queue.pushCommand(std::make_unique<Commands::ConnectCommand>());
    queue.pushCommand(std::make_unique<Commands::DisconnectCommand>());
    queue.pushCommand(std::make_unique<Commands::ShutdownCommand>());

    EXPECT_EQ(queue.size(), 3u);

    // Pop in FIFO order
    auto cmd1 = queue.waitAndPop(100);
    ASSERT_NE(cmd1, nullptr);
    EXPECT_EQ(cmd1->type, Commands::Command::Connect);

    auto cmd2 = queue.waitAndPop(100);
    ASSERT_NE(cmd2, nullptr);
    EXPECT_EQ(cmd2->type, Commands::Command::Disconnect);

    auto cmd3 = queue.waitAndPop(100);
    ASSERT_NE(cmd3, nullptr);
    EXPECT_EQ(cmd3->type, Commands::Command::Shutdown);

    EXPECT_EQ(queue.size(), 0u);
}

TEST(NetworkConnectionQueueTest, TimeoutOnEmptyQueue) {
    NetworkConnectionQueue queue;

    // Attempt to pop from empty queue with timeout
    auto start = std::chrono::high_resolution_clock::now();
    auto cmd = queue.waitAndPop(50);  // 50ms timeout
    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_EQ(cmd, nullptr);  // Should return nullptr

    // Verify timeout occurred (allow 10ms tolerance)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(duration.count(), 45);
    EXPECT_LE(duration.count(), 100);
}

TEST(NetworkConnectionQueueTest, CommandPolymorphism) {
    NetworkConnectionQueue queue;

    // Push command with parameters
    std::vector<uint8_t> midiData = {0x90, 0x3C, 0x64};  // Note On
    queue.pushCommand(std::make_unique<Commands::SendMidiCommand>(1, midiData));

    auto cmd = queue.waitAndPop(100);
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->type, Commands::Command::SendMidi);

    // Downcast to specific type
    auto* midiCmd = static_cast<Commands::SendMidiCommand*>(cmd.get());
    EXPECT_EQ(midiCmd->deviceId, 1u);
    EXPECT_EQ(midiCmd->data, midiData);
}

//==============================================================================
// Multi-threaded stress tests

TEST(NetworkConnectionQueueTest, MultiProducerSingleConsumer) {
    NetworkConnectionQueue queue;
    std::atomic<int> commandsProduced{0};
    std::atomic<int> commandsConsumed{0};

    const int NUM_PRODUCERS = 10;
    const int COMMANDS_PER_PRODUCER = 100;
    const int TOTAL_COMMANDS = NUM_PRODUCERS * COMMANDS_PER_PRODUCER;

    // Start producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back([&queue, &commandsProduced, COMMANDS_PER_PRODUCER]() {
            for (int j = 0; j < COMMANDS_PER_PRODUCER; ++j) {
                queue.pushCommand(std::make_unique<Commands::ConnectCommand>());
                commandsProduced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Consumer thread
    std::thread consumer([&queue, &commandsConsumed, TOTAL_COMMANDS]() {
        for (int i = 0; i < TOTAL_COMMANDS; ++i) {
            auto cmd = queue.waitAndPop(1000);  // 1 second timeout
            if (cmd) {
                commandsConsumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                // Timeout - should not happen
                break;
            }
        }
    });

    // Wait for all threads
    for (auto& t : producers) {
        t.join();
    }
    consumer.join();

    // Verify all commands produced and consumed
    EXPECT_EQ(commandsProduced.load(), TOTAL_COMMANDS);
    EXPECT_EQ(commandsConsumed.load(), TOTAL_COMMANDS);
    EXPECT_EQ(queue.size(), 0u);
}

TEST(NetworkConnectionQueueTest, HighFrequencyOperations) {
    NetworkConnectionQueue queue;
    std::atomic<bool> stopFlag{false};
    std::atomic<int> pushCount{0};
    std::atomic<int> popCount{0};

    // Producer thread - push commands rapidly
    std::thread producer([&]() {
        while (!stopFlag.load(std::memory_order_acquire)) {
            queue.pushCommand(std::make_unique<Commands::ConnectCommand>());
            pushCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Consumer thread - pop commands rapidly
    std::thread consumer([&]() {
        while (!stopFlag.load(std::memory_order_acquire)) {
            auto cmd = queue.waitAndPop(10);  // 10ms timeout
            if (cmd) {
                popCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        // Drain remaining commands
        while (queue.size() > 0) {
            auto cmd = queue.waitAndPop(10);
            if (cmd) {
                popCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Run for 1 second
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    stopFlag.store(true, std::memory_order_release);

    producer.join();
    consumer.join();

    // Verify balance
    EXPECT_EQ(pushCount.load(), popCount.load());
    EXPECT_GT(pushCount.load(), 1000);  // Should have processed many commands

    std::cout << "High frequency test: " << pushCount.load()
              << " commands processed in 1 second" << std::endl;
}

//==============================================================================
// Query command with response mechanism

TEST(NetworkConnectionQueueTest, QueryCommandWithResponse) {
    NetworkConnectionQueue queue;

    // Worker thread simulating command processing
    std::thread worker([&queue]() {
        auto cmd = queue.waitAndPop(1000);
        ASSERT_NE(cmd, nullptr);
        EXPECT_EQ(cmd->type, Commands::Command::GetState);

        // Process query
        auto* query = static_cast<Commands::GetStateQuery*>(cmd.get());
        query->result = NetworkConnection::State::Connected;
        query->signal();
    });

    // Main thread sends query and waits for response
    auto query = std::make_unique<Commands::GetStateQuery>();
    auto* queryPtr = query.get();

    queue.pushCommand(std::move(query));

    // Wait for response (with timeout)
    bool gotResponse = queryPtr->wait(1000);
    EXPECT_TRUE(gotResponse);
    EXPECT_EQ(queryPtr->result, NetworkConnection::State::Connected);

    worker.join();
}

TEST(NetworkConnectionQueueTest, MultipleQueriesConcurrent) {
    NetworkConnectionQueue queue;
    const int NUM_QUERIES = 50;
    std::atomic<int> queriesProcessed{0};

    // Worker thread processes queries
    std::thread worker([&]() {
        while (queriesProcessed.load() < NUM_QUERIES) {
            auto cmd = queue.waitAndPop(100);
            if (cmd && cmd->type == Commands::Command::GetRemoteNode) {
                auto* query = static_cast<Commands::GetRemoteNodeQuery*>(cmd.get());

                // Simulate some processing
                query->result.name = "TestNode";
                query->result.uuid = juce::Uuid();

                query->signal();
                queriesProcessed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    // Multiple threads send queries
    std::vector<std::thread> queryThreads;
    for (int i = 0; i < NUM_QUERIES; ++i) {
        queryThreads.emplace_back([&queue]() {
            auto query = std::make_unique<Commands::GetRemoteNodeQuery>();
            auto* queryPtr = query.get();

            queue.pushCommand(std::move(query));

            bool gotResponse = queryPtr->wait(2000);
            EXPECT_TRUE(gotResponse);
            EXPECT_EQ(queryPtr->result.name, "TestNode");
        });
    }

    for (auto& t : queryThreads) {
        t.join();
    }

    worker.join();

    EXPECT_EQ(queriesProcessed.load(), NUM_QUERIES);
}

//==============================================================================
// Shutdown behavior

TEST(NetworkConnectionQueueTest, ShutdownCommand) {
    NetworkConnectionQueue queue;
    std::atomic<bool> workerStopped{false};

    // Worker thread waits for shutdown
    std::thread worker([&]() {
        while (true) {
            auto cmd = queue.waitAndPop(100);
            if (cmd && cmd->type == Commands::Command::Shutdown) {
                break;
            }
        }
        workerStopped.store(true);
    });

    // Push some commands
    queue.pushCommand(std::make_unique<Commands::ConnectCommand>());
    queue.pushCommand(std::make_unique<Commands::DisconnectCommand>());

    // Push shutdown command
    queue.pushCommand(std::make_unique<Commands::ShutdownCommand>());

    worker.join();

    EXPECT_TRUE(workerStopped.load());
}
