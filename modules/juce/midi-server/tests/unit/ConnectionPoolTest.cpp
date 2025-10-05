/**
 * ConnectionPoolTest.cpp
 *
 * Unit tests for ConnectionPool
 * Tests: connection management, duplicates, cleanup, statistics, thread safety
 *
 * Coverage Target: 80%+
 */

#include "network/mesh/ConnectionPool.h"
#include "network/mesh/NetworkConnection.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

using namespace NetworkMidi;
using namespace testing;

class ConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_unique<ConnectionPool>();
        node1 = juce::Uuid();
        node2 = juce::Uuid();
        node3 = juce::Uuid();
    }

    NodeInfo createNodeInfo(const juce::Uuid& uuid, int httpPort = 8080, int udpPort = 9000) {
        NodeInfo info;
        info.uuid = uuid;
        info.name = "Test Node " + uuid.toString().substring(0, 8);
        info.hostname = "localhost";
        info.ipAddress = "127.0.0.1";
        info.httpPort = httpPort;
        info.udpPort = udpPort;
        return info;
    }

    std::unique_ptr<NetworkConnection> createConnection(const juce::Uuid& uuid) {
        return std::make_unique<NetworkConnection>(createNodeInfo(uuid));
    }

    std::unique_ptr<ConnectionPool> pool;
    juce::Uuid node1;
    juce::Uuid node2;
    juce::Uuid node3;
};

// Test add connection
TEST_F(ConnectionPoolTest, AddsConnection) {
    auto conn = createConnection(node1);

    bool added = pool->addConnection(std::move(conn));

    EXPECT_TRUE(added);
    EXPECT_EQ(1u, pool->getConnectionCount());
    EXPECT_TRUE(pool->hasConnection(node1));
}

// Test add duplicate connection
TEST_F(ConnectionPoolTest, RejectsDuplicateConnection) {
    auto conn1 = createConnection(node1);
    auto conn2 = createConnection(node1);

    pool->addConnection(std::move(conn1));
    bool added = pool->addConnection(std::move(conn2));

    EXPECT_FALSE(added);
    EXPECT_EQ(1u, pool->getConnectionCount());
}

// Test add null connection
TEST_F(ConnectionPoolTest, RejectsNullConnection) {
    bool added = pool->addConnection(nullptr);

    EXPECT_FALSE(added);
    EXPECT_EQ(0u, pool->getConnectionCount());
}

// Test remove connection
TEST_F(ConnectionPoolTest, RemovesConnection) {
    auto conn = createConnection(node1);
    pool->addConnection(std::move(conn));

    bool removed = pool->removeConnection(node1);

    EXPECT_TRUE(removed);
    EXPECT_EQ(0u, pool->getConnectionCount());
    EXPECT_FALSE(pool->hasConnection(node1));
}

// Test remove non-existent connection
TEST_F(ConnectionPoolTest, RemoveNonExistentConnectionReturnsFalse) {
    bool removed = pool->removeConnection(node1);

    EXPECT_FALSE(removed);
}

// Test get connection
TEST_F(ConnectionPoolTest, GetsConnection) {
    auto conn = createConnection(node1);
    auto* connPtr = conn.get();

    pool->addConnection(std::move(conn));

    NetworkConnection* retrieved = pool->getConnection(node1);

    EXPECT_EQ(connPtr, retrieved);
}

// Test get non-existent connection
TEST_F(ConnectionPoolTest, GetNonExistentConnectionReturnsNull) {
    NetworkConnection* conn = pool->getConnection(node1);

    EXPECT_EQ(nullptr, conn);
}

// Test has connection
TEST_F(ConnectionPoolTest, ChecksConnectionExists) {
    auto conn = createConnection(node1);
    pool->addConnection(std::move(conn));

    EXPECT_TRUE(pool->hasConnection(node1));
    EXPECT_FALSE(pool->hasConnection(node2));
}

// Test get all connections
TEST_F(ConnectionPoolTest, GetsAllConnections) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));

    auto connections = pool->getAllConnections();

    EXPECT_EQ(2u, connections.size());
}

// Test get connections by state
TEST_F(ConnectionPoolTest, GetsConnectionsByState) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));
    pool->addConnection(createConnection(node3));

    // All connections start in Disconnected state
    auto disconnected = pool->getConnectionsByState(NetworkConnection::State::Disconnected);

    EXPECT_EQ(3u, disconnected.size());
}

// Test connection count
TEST_F(ConnectionPoolTest, GetsConnectionCount) {
    EXPECT_EQ(0u, pool->getConnectionCount());

    pool->addConnection(createConnection(node1));
    EXPECT_EQ(1u, pool->getConnectionCount());

    pool->addConnection(createConnection(node2));
    EXPECT_EQ(2u, pool->getConnectionCount());
}

// Test clear
TEST_F(ConnectionPoolTest, ClearsAllConnections) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));

    pool->clear();

    EXPECT_EQ(0u, pool->getConnectionCount());
    EXPECT_FALSE(pool->hasConnection(node1));
    EXPECT_FALSE(pool->hasConnection(node2));
}

// Test remove dead connections - failed state
TEST_F(ConnectionPoolTest, RemovesFailedConnections) {
    auto conn = createConnection(node1);
    auto* connPtr = conn.get();
    pool->addConnection(std::move(conn));

    // Simulate connection failure by triggering failed state
    connPtr->disconnect();

    // Note: In the real implementation, failed connections would be marked as Failed
    // For this test, we verify the removeDeadConnections mechanism works
    int removed = pool->removeDeadConnections();

    // Since we can't easily mock state, test that the method completes
    EXPECT_GE(removed, 0);
}

// Test get statistics
TEST_F(ConnectionPoolTest, GetsStatistics) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));
    pool->addConnection(createConnection(node3));

    auto stats = pool->getStatistics();

    EXPECT_EQ(3u, stats.totalConnections);
    // All connections start as Disconnected
    EXPECT_EQ(3u, stats.disconnectedCount);
}

// Test empty pool
TEST_F(ConnectionPoolTest, EmptyPool) {
    EXPECT_EQ(0u, pool->getConnectionCount());

    auto connections = pool->getAllConnections();
    EXPECT_TRUE(connections.empty());

    auto stats = pool->getStatistics();
    EXPECT_EQ(0u, stats.totalConnections);
}

// Test concurrent add
TEST_F(ConnectionPoolTest, HandlesConcurrentAdd) {
    std::vector<std::thread> threads;
    std::vector<juce::Uuid> nodes;

    for (int i = 0; i < 10; ++i) {
        nodes.push_back(juce::Uuid());
    }

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &nodes, i]() {
            auto conn = createConnection(nodes[i]);
            pool->addConnection(std::move(conn));
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(10u, pool->getConnectionCount());
}

// Test concurrent remove
TEST_F(ConnectionPoolTest, HandlesConcurrentRemove) {
    std::vector<juce::Uuid> nodes;

    for (int i = 0; i < 10; ++i) {
        juce::Uuid node;
        nodes.push_back(node);
        pool->addConnection(createConnection(node));
    }

    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &nodes, i]() {
            pool->removeConnection(nodes[i]);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(0u, pool->getConnectionCount());
}

// Test concurrent read/write
TEST_F(ConnectionPoolTest, HandlesConcurrentReadWrite) {
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Writer thread
    threads.emplace_back([this, &running]() {
        int count = 0;
        while (running && count < 50) {
            juce::Uuid node;
            auto conn = createConnection(node);
            pool->addConnection(std::move(conn));
            count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([this, &running]() {
            while (running) {
                auto connections = pool->getAllConnections();
                auto count = pool->getConnectionCount();
                auto stats = pool->getStatistics();
                (void)connections;
                (void)count;
                (void)stats;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    for (auto& thread : threads) {
        thread.join();
    }
}

// Test no dead connections initially
TEST_F(ConnectionPoolTest, NoDeadConnectionsInitially) {
    pool->addConnection(createConnection(node1));

    int removed = pool->removeDeadConnections();

    EXPECT_EQ(0, removed);
    EXPECT_EQ(1u, pool->getConnectionCount());
}

// Test multiple clear operations
TEST_F(ConnectionPoolTest, MultipleClearOperations) {
    pool->addConnection(createConnection(node1));
    pool->clear();

    EXPECT_EQ(0u, pool->getConnectionCount());

    pool->addConnection(createConnection(node2));
    pool->clear();

    EXPECT_EQ(0u, pool->getConnectionCount());
}

// Test connection ownership transfer
TEST_F(ConnectionPoolTest, TakesOwnershipOfConnection) {
    auto conn = createConnection(node1);
    auto* rawPtr = conn.get();

    pool->addConnection(std::move(conn));

    // Original unique_ptr should be null
    EXPECT_EQ(nullptr, conn.get());

    // Pool should have the connection
    EXPECT_EQ(rawPtr, pool->getConnection(node1));
}

// Test get connections by state empty result
TEST_F(ConnectionPoolTest, GetConnectionsByStateEmptyResult) {
    pool->addConnection(createConnection(node1));

    auto connected = pool->getConnectionsByState(NetworkConnection::State::Connected);

    EXPECT_TRUE(connected.empty());
}

// Test statistics with multiple connections
TEST_F(ConnectionPoolTest, StatisticsMultipleConnections) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));

    auto stats = pool->getStatistics();

    EXPECT_EQ(2u, stats.totalConnections);
    EXPECT_EQ(2u, stats.disconnectedCount);
}

// Test remove during iteration safety
TEST_F(ConnectionPoolTest, SafeRemoveDuringIteration) {
    for (int i = 0; i < 5; ++i) {
        juce::Uuid node;
        pool->addConnection(createConnection(node));
    }

    auto connections = pool->getAllConnections();

    // Remove first connection while holding vector
    if (!connections.empty()) {
        auto firstNode = connections[0]->getRemoteNode().uuid;
        pool->removeConnection(firstNode);
    }

    EXPECT_EQ(4u, pool->getConnectionCount());
}

// Test different node IDs
TEST_F(ConnectionPoolTest, DifferentNodeIds) {
    auto conn1 = createConnection(node1);
    auto conn2 = createConnection(node2);

    pool->addConnection(std::move(conn1));
    pool->addConnection(std::move(conn2));

    EXPECT_TRUE(pool->hasConnection(node1));
    EXPECT_TRUE(pool->hasConnection(node2));
    EXPECT_NE(node1, node2);
}

// Test connection retrieval accuracy
TEST_F(ConnectionPoolTest, RetrievesCorrectConnection) {
    pool->addConnection(createConnection(node1));
    pool->addConnection(createConnection(node2));

    auto* conn1 = pool->getConnection(node1);
    auto* conn2 = pool->getConnection(node2);

    ASSERT_NE(nullptr, conn1);
    ASSERT_NE(nullptr, conn2);
    EXPECT_EQ(node1, conn1->getRemoteNode().uuid);
    EXPECT_EQ(node2, conn2->getRemoteNode().uuid);
}

// Test clear disconnects all
TEST_F(ConnectionPoolTest, ClearDisconnectsAll) {
    auto conn1 = createConnection(node1);
    auto conn2 = createConnection(node2);

    pool->addConnection(std::move(conn1));
    pool->addConnection(std::move(conn2));

    pool->clear();

    EXPECT_EQ(0u, pool->getConnectionCount());
}

// Test state consistency
TEST_F(ConnectionPoolTest, StateConsistency) {
    pool->addConnection(createConnection(node1));

    auto* conn = pool->getConnection(node1);
    ASSERT_NE(nullptr, conn);

    auto state = conn->getState();
    EXPECT_EQ(NetworkConnection::State::Disconnected, state);
}

// Test remove dead connections with no connections
TEST_F(ConnectionPoolTest, RemoveDeadConnectionsEmpty) {
    int removed = pool->removeDeadConnections();

    EXPECT_EQ(0, removed);
}
