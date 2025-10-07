/**
 * RoutingTableTest.cpp
 *
 * Unit tests for RoutingTable
 * Tests: route add/remove, bulk operations, lookups, thread safety
 *
 * Coverage Target: 80%+
 */

#include "network/routing/RoutingTable.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

using namespace NetworkMidi;
using namespace testing;

class RoutingTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        table = std::make_unique<RoutingTable>();
        localNode = juce::Uuid::null();
        remoteNode1 = juce::Uuid();
        remoteNode2 = juce::Uuid();
    }

    std::unique_ptr<RoutingTable> table;
    juce::Uuid localNode;
    juce::Uuid remoteNode1;
    juce::Uuid remoteNode2;
};

// Test add local route
TEST_F(RoutingTableTest, AddsLocalRoute) {
    table->addRoute(localNode, 1, "Local Device", "input");

    auto route = table->getLocalRoute(1);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(1, route->deviceId());
    EXPECT_TRUE(route->isLocal());
    EXPECT_EQ(juce::String("Local Device"), route->deviceName);
    EXPECT_EQ(juce::String("input"), route->deviceType);
}

// Test add remote route
TEST_F(RoutingTableTest, AddsRemoteRoute) {
    table->addRoute(remoteNode1, 2, "Remote Device", "output");

    auto route = table->getRoute(remoteNode1, 2);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(2, route->deviceId());
    EXPECT_FALSE(route->isLocal());
    EXPECT_EQ(remoteNode1, route->nodeId());
    EXPECT_EQ(juce::String("Remote Device"), route->deviceName);
}

// Test remove route
TEST_F(RoutingTableTest, RemovesRoute) {
    table->addRoute(localNode, 1, "Device", "input");

    table->removeRoute(localNode, 1);

    auto route = table->getLocalRoute(1);
    EXPECT_FALSE(route.has_value());
}

// Test remove node routes
TEST_F(RoutingTableTest, RemovesNodeRoutes) {
    table->addRoute(remoteNode1, 1, "Node1 Device1", "input");
    table->addRoute(remoteNode1, 2, "Node1 Device2", "output");
    table->addRoute(remoteNode2, 3, "Node2 Device", "input");

    table->removeNodeRoutes(remoteNode1);

    EXPECT_FALSE(table->getRoute(remoteNode1, 1).has_value());
    EXPECT_FALSE(table->getRoute(remoteNode1, 2).has_value());
    EXPECT_TRUE(table->getRoute(remoteNode2, 3).has_value());
}

// Test clear all routes
TEST_F(RoutingTableTest, ClearsAllRoutes) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    table->clearAllRoutes();

    EXPECT_FALSE(table->getLocalRoute(1).has_value());
    EXPECT_FALSE(table->getRoute(remoteNode1, 2).has_value());
    EXPECT_EQ(0, table->getTotalRouteCount());
}

// Test get all routes
TEST_F(RoutingTableTest, GetsAllRoutes) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    auto routes = table->getAllRoutes();

    EXPECT_EQ(2u, routes.size());
}

// Test get local routes
TEST_F(RoutingTableTest, GetsLocalRoutes) {
    table->addRoute(localNode, 1, "Local 1", "input");
    table->addRoute(localNode, 2, "Local 2", "output");
    table->addRoute(remoteNode1, 3, "Remote", "input");

    auto routes = table->getLocalRoutes();

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_TRUE(route.isLocal());
    }
}

// Test get remote routes
TEST_F(RoutingTableTest, GetsRemoteRoutes) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote 1", "output");
    table->addRoute(remoteNode2, 3, "Remote 2", "input");

    auto routes = table->getRemoteRoutes();

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_FALSE(route.isLocal());
    }
}

// Test get node routes
TEST_F(RoutingTableTest, GetsNodeRoutes) {
    table->addRoute(remoteNode1, 1, "Node1 Device1", "input");
    table->addRoute(remoteNode1, 2, "Node1 Device2", "output");
    table->addRoute(remoteNode2, 3, "Node2 Device", "input");

    auto routes = table->getNodeRoutes(remoteNode1);

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_EQ(remoteNode1, route.nodeId());
    }
}

// Test has route
TEST_F(RoutingTableTest, ChecksRouteExists) {
    table->addRoute(localNode, 1, "Device", "input");

    EXPECT_TRUE(table->hasLocalRoute(1));
    EXPECT_FALSE(table->hasLocalRoute(999));
}

// Test is local device
TEST_F(RoutingTableTest, ChecksIsLocalDevice) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    EXPECT_TRUE(table->isLocalDevice(localNode, 1));
    EXPECT_FALSE(table->isLocalDevice(remoteNode1, 2));
    EXPECT_FALSE(table->isLocalDevice(localNode, 999));  // Non-existent
}

// Test is remote device
TEST_F(RoutingTableTest, ChecksIsRemoteDevice) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    EXPECT_FALSE(table->isRemoteDevice(localNode, 1));
    EXPECT_TRUE(table->isRemoteDevice(remoteNode1, 2));
    EXPECT_FALSE(table->isRemoteDevice(localNode, 999));  // Non-existent
}

// Test total route count
TEST_F(RoutingTableTest, GetsTotalRouteCount) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    EXPECT_EQ(2, table->getTotalRouteCount());
}

// Test local route count
TEST_F(RoutingTableTest, GetsLocalRouteCount) {
    table->addRoute(localNode, 1, "Local 1", "input");
    table->addRoute(localNode, 2, "Local 2", "output");
    table->addRoute(remoteNode1, 3, "Remote", "input");

    EXPECT_EQ(2, table->getLocalRouteCount());
}

// Test remote route count
TEST_F(RoutingTableTest, GetsRemoteRouteCount) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote 1", "output");
    table->addRoute(remoteNode2, 3, "Remote 2", "input");

    EXPECT_EQ(2, table->getRemoteRouteCount());
}

// Test node route count
TEST_F(RoutingTableTest, GetsNodeRouteCount) {
    table->addRoute(remoteNode1, 1, "Node1 Device1", "input");
    table->addRoute(remoteNode1, 2, "Node1 Device2", "output");
    table->addRoute(remoteNode2, 3, "Node2 Device", "input");

    EXPECT_EQ(2, table->getNodeRouteCount(remoteNode1));
    EXPECT_EQ(1, table->getNodeRouteCount(remoteNode2));
}

// Test add routes bulk
TEST_F(RoutingTableTest, AddsBulkRoutes) {
    std::vector<Route> routes;
    routes.push_back(Route(localNode, 1, "Device 1", "input"));
    routes.push_back(Route(remoteNode1, 2, "Device 2", "output"));
    routes.push_back(Route(remoteNode1, 3, "Device 3", "input"));

    table->addRoutes(routes);

    EXPECT_EQ(3, table->getTotalRouteCount());
    EXPECT_TRUE(table->hasLocalRoute(1));
    EXPECT_TRUE(table->hasRoute(remoteNode1, 2));
    EXPECT_TRUE(table->hasRoute(remoteNode1, 3));
}

// Test replace node routes
TEST_F(RoutingTableTest, ReplacesNodeRoutes) {
    table->addRoute(remoteNode1, 1, "Old Device 1", "input");
    table->addRoute(remoteNode1, 2, "Old Device 2", "output");

    std::vector<Route> newRoutes;
    newRoutes.push_back(Route(remoteNode1, 3, "New Device 1", "input"));
    newRoutes.push_back(Route(remoteNode1, 4, "New Device 2", "output"));

    table->replaceNodeRoutes(remoteNode1, newRoutes);

    EXPECT_FALSE(table->hasRoute(remoteNode1, 1));
    EXPECT_FALSE(table->hasRoute(remoteNode1, 2));
    EXPECT_TRUE(table->hasRoute(remoteNode1, 3));
    EXPECT_TRUE(table->hasRoute(remoteNode1, 4));
    EXPECT_EQ(2, table->getNodeRouteCount(remoteNode1));
}

// Test update existing route
TEST_F(RoutingTableTest, UpdatesExistingRoute) {
    table->addRoute(localNode, 1, "Original Name", "input");

    table->addRoute(localNode, 1, "Updated Name", "output");

    auto route = table->getLocalRoute(1);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(juce::String("Updated Name"), route->deviceName);
    EXPECT_EQ(juce::String("output"), route->deviceType);
    EXPECT_TRUE(route->isLocal());
    EXPECT_EQ(localNode, route->nodeId());
}

// Test empty table
TEST_F(RoutingTableTest, EmptyTable) {
    EXPECT_EQ(0, table->getTotalRouteCount());
    EXPECT_EQ(0, table->getLocalRouteCount());
    EXPECT_EQ(0, table->getRemoteRouteCount());

    auto routes = table->getAllRoutes();
    EXPECT_TRUE(routes.empty());
}

// Test route equality
TEST_F(RoutingTableTest, RouteEquality) {
    Route route1(localNode, 1, "Device", "input");
    Route route2(remoteNode1, 1, "Different", "output");
    Route route3(localNode, 2, "Device", "input");

    EXPECT_FALSE(route1 == route2);  // Different node ID
    EXPECT_FALSE(route1 == route3);  // Different device ID
}

// Test route isLocal
TEST_F(RoutingTableTest, RouteIsLocal) {
    Route localRoute(juce::Uuid::null(), 1, "Local", "input");
    Route remoteRoute(remoteNode1, 2, "Remote", "output");

    EXPECT_TRUE(localRoute.isLocal());
    EXPECT_FALSE(remoteRoute.isLocal());
}

// Test concurrent route addition
TEST_F(RoutingTableTest, HandlesConcurrentAddition) {
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            table->addRoute(remoteNode1, i, "Device " + juce::String(i), "input");
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(10, table->getTotalRouteCount());
}

// Test concurrent route removal
TEST_F(RoutingTableTest, HandlesConcurrentRemoval) {
    // Add routes
    for (int i = 0; i < 10; ++i) {
        table->addRoute(remoteNode1, i, "Device " + juce::String(i), "input");
    }

    // Remove concurrently
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            table->removeRoute(remoteNode1, i);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(0, table->getTotalRouteCount());
}

// Test concurrent read/write
TEST_F(RoutingTableTest, HandlesConcurrentReadWrite) {
    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Writer thread
    threads.emplace_back([this, &running]() {
        uint16_t id = 0;
        while (running) {
            table->addRoute(remoteNode1, id++, "Device", "input");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([this, &running]() {
            while (running) {
                auto routes = table->getAllRoutes();
                auto count = table->getTotalRouteCount();
                (void)routes;
                (void)count;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    for (auto& thread : threads) {
        thread.join();
    }
}

// Test get non-existent route
TEST_F(RoutingTableTest, GetNonExistentRoute) {
    auto route = table->getLocalRoute(999);

    EXPECT_FALSE(route.has_value());
}

// Test empty bulk add
TEST_F(RoutingTableTest, HandlesEmptyBulkAdd) {
    std::vector<Route> emptyRoutes;

    table->addRoutes(emptyRoutes);

    EXPECT_EQ(0, table->getTotalRouteCount());
}

// Test replace with empty routes
TEST_F(RoutingTableTest, ReplacesWithEmptyRoutes) {
    table->addRoute(remoteNode1, 1, "Device 1", "input");
    table->addRoute(remoteNode1, 2, "Device 2", "output");

    std::vector<Route> emptyRoutes;
    table->replaceNodeRoutes(remoteNode1, emptyRoutes);

    EXPECT_EQ(0, table->getNodeRouteCount(remoteNode1));
}

// Test local node identification
TEST_F(RoutingTableTest, IdentifiesLocalNode) {
    Route route(juce::Uuid::null(), 1, "Device", "input");

    EXPECT_TRUE(route.nodeId().isNull());
    EXPECT_TRUE(route.isLocal());
}

// Test device type preservation
TEST_F(RoutingTableTest, PreservesDeviceType) {
    table->addRoute(localNode, 1, "Input Device", "input");
    table->addRoute(localNode, 2, "Output Device", "output");

    auto inputRoute = table->getLocalRoute(1);
    auto outputRoute = table->getLocalRoute(2);

    ASSERT_TRUE(inputRoute.has_value());
    ASSERT_TRUE(outputRoute.has_value());
    EXPECT_EQ(juce::String("input"), inputRoute->deviceType);
    EXPECT_EQ(juce::String("output"), outputRoute->deviceType);
}

// Test node routes for non-existent node
TEST_F(RoutingTableTest, GetsEmptyNodeRoutes) {
    juce::Uuid nonExistentNode;

    auto routes = table->getNodeRoutes(nonExistentNode);

    EXPECT_TRUE(routes.empty());
}

// Test mixed local and remote routes
TEST_F(RoutingTableTest, HandlesMixedRoutes) {
    table->addRoute(localNode, 1, "Local", "input");
    table->addRoute(remoteNode1, 2, "Remote", "output");

    EXPECT_EQ(2, table->getTotalRouteCount());
    EXPECT_EQ(1, table->getLocalRouteCount());
    EXPECT_EQ(1, table->getRemoteRouteCount());
}
