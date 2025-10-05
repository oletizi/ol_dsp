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
    table->addRoute(1, localNode, "Local Device", "input");

    auto route = table->getRoute(1);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(1, route->deviceId);
    EXPECT_TRUE(route->isLocal());
    EXPECT_EQ("Local Device", route->deviceName);
    EXPECT_EQ("input", route->deviceType);
}

// Test add remote route
TEST_F(RoutingTableTest, AddsRemoteRoute) {
    table->addRoute(2, remoteNode1, "Remote Device", "output");

    auto route = table->getRoute(2);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(2, route->deviceId);
    EXPECT_FALSE(route->isLocal());
    EXPECT_EQ(remoteNode1, route->nodeId);
    EXPECT_EQ("Remote Device", route->deviceName);
}

// Test remove route
TEST_F(RoutingTableTest, RemovesRoute) {
    table->addRoute(1, localNode, "Device", "input");

    table->removeRoute(1);

    auto route = table->getRoute(1);
    EXPECT_FALSE(route.has_value());
}

// Test remove node routes
TEST_F(RoutingTableTest, RemovesNodeRoutes) {
    table->addRoute(1, remoteNode1, "Node1 Device1", "input");
    table->addRoute(2, remoteNode1, "Node1 Device2", "output");
    table->addRoute(3, remoteNode2, "Node2 Device", "input");

    table->removeNodeRoutes(remoteNode1);

    EXPECT_FALSE(table->getRoute(1).has_value());
    EXPECT_FALSE(table->getRoute(2).has_value());
    EXPECT_TRUE(table->getRoute(3).has_value());
}

// Test clear all routes
TEST_F(RoutingTableTest, ClearsAllRoutes) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    table->clearAllRoutes();

    EXPECT_FALSE(table->getRoute(1).has_value());
    EXPECT_FALSE(table->getRoute(2).has_value());
    EXPECT_EQ(0, table->getTotalRouteCount());
}

// Test get all routes
TEST_F(RoutingTableTest, GetsAllRoutes) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    auto routes = table->getAllRoutes();

    EXPECT_EQ(2u, routes.size());
}

// Test get local routes
TEST_F(RoutingTableTest, GetsLocalRoutes) {
    table->addRoute(1, localNode, "Local 1", "input");
    table->addRoute(2, localNode, "Local 2", "output");
    table->addRoute(3, remoteNode1, "Remote", "input");

    auto routes = table->getLocalRoutes();

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_TRUE(route.isLocal());
    }
}

// Test get remote routes
TEST_F(RoutingTableTest, GetsRemoteRoutes) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote 1", "output");
    table->addRoute(3, remoteNode2, "Remote 2", "input");

    auto routes = table->getRemoteRoutes();

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_FALSE(route.isLocal());
    }
}

// Test get node routes
TEST_F(RoutingTableTest, GetsNodeRoutes) {
    table->addRoute(1, remoteNode1, "Node1 Device1", "input");
    table->addRoute(2, remoteNode1, "Node1 Device2", "output");
    table->addRoute(3, remoteNode2, "Node2 Device", "input");

    auto routes = table->getNodeRoutes(remoteNode1);

    EXPECT_EQ(2u, routes.size());
    for (const auto& route : routes) {
        EXPECT_EQ(remoteNode1, route.nodeId);
    }
}

// Test has route
TEST_F(RoutingTableTest, ChecksRouteExists) {
    table->addRoute(1, localNode, "Device", "input");

    EXPECT_TRUE(table->hasRoute(1));
    EXPECT_FALSE(table->hasRoute(999));
}

// Test is local device
TEST_F(RoutingTableTest, ChecksIsLocalDevice) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    EXPECT_TRUE(table->isLocalDevice(1));
    EXPECT_FALSE(table->isLocalDevice(2));
    EXPECT_FALSE(table->isLocalDevice(999));  // Non-existent
}

// Test is remote device
TEST_F(RoutingTableTest, ChecksIsRemoteDevice) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    EXPECT_FALSE(table->isRemoteDevice(1));
    EXPECT_TRUE(table->isRemoteDevice(2));
    EXPECT_FALSE(table->isRemoteDevice(999));  // Non-existent
}

// Test total route count
TEST_F(RoutingTableTest, GetsTotalRouteCount) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    EXPECT_EQ(2, table->getTotalRouteCount());
}

// Test local route count
TEST_F(RoutingTableTest, GetsLocalRouteCount) {
    table->addRoute(1, localNode, "Local 1", "input");
    table->addRoute(2, localNode, "Local 2", "output");
    table->addRoute(3, remoteNode1, "Remote", "input");

    EXPECT_EQ(2, table->getLocalRouteCount());
}

// Test remote route count
TEST_F(RoutingTableTest, GetsRemoteRouteCount) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote 1", "output");
    table->addRoute(3, remoteNode2, "Remote 2", "input");

    EXPECT_EQ(2, table->getRemoteRouteCount());
}

// Test node route count
TEST_F(RoutingTableTest, GetsNodeRouteCount) {
    table->addRoute(1, remoteNode1, "Node1 Device1", "input");
    table->addRoute(2, remoteNode1, "Node1 Device2", "output");
    table->addRoute(3, remoteNode2, "Node2 Device", "input");

    EXPECT_EQ(2, table->getNodeRouteCount(remoteNode1));
    EXPECT_EQ(1, table->getNodeRouteCount(remoteNode2));
}

// Test add routes bulk
TEST_F(RoutingTableTest, AddsBulkRoutes) {
    std::vector<Route> routes;
    routes.push_back(Route(1, localNode, "Device 1", "input"));
    routes.push_back(Route(2, remoteNode1, "Device 2", "output"));
    routes.push_back(Route(3, remoteNode1, "Device 3", "input"));

    table->addRoutes(routes);

    EXPECT_EQ(3, table->getTotalRouteCount());
    EXPECT_TRUE(table->hasRoute(1));
    EXPECT_TRUE(table->hasRoute(2));
    EXPECT_TRUE(table->hasRoute(3));
}

// Test replace node routes
TEST_F(RoutingTableTest, ReplacesNodeRoutes) {
    table->addRoute(1, remoteNode1, "Old Device 1", "input");
    table->addRoute(2, remoteNode1, "Old Device 2", "output");

    std::vector<Route> newRoutes;
    newRoutes.push_back(Route(3, remoteNode1, "New Device 1", "input"));
    newRoutes.push_back(Route(4, remoteNode1, "New Device 2", "output"));

    table->replaceNodeRoutes(remoteNode1, newRoutes);

    EXPECT_FALSE(table->hasRoute(1));
    EXPECT_FALSE(table->hasRoute(2));
    EXPECT_TRUE(table->hasRoute(3));
    EXPECT_TRUE(table->hasRoute(4));
    EXPECT_EQ(2, table->getNodeRouteCount(remoteNode1));
}

// Test update existing route
TEST_F(RoutingTableTest, UpdatesExistingRoute) {
    table->addRoute(1, localNode, "Original Name", "input");

    table->addRoute(1, remoteNode1, "Updated Name", "output");

    auto route = table->getRoute(1);

    ASSERT_TRUE(route.has_value());
    EXPECT_EQ("Updated Name", route->deviceName);
    EXPECT_EQ("output", route->deviceType);
    EXPECT_FALSE(route->isLocal());
    EXPECT_EQ(remoteNode1, route->nodeId);
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
    Route route1(1, localNode, "Device", "input");
    Route route2(1, remoteNode1, "Different", "output");
    Route route3(2, localNode, "Device", "input");

    EXPECT_TRUE(route1 == route2);  // Same device ID
    EXPECT_FALSE(route1 == route3);  // Different device ID
}

// Test route isLocal
TEST_F(RoutingTableTest, RouteIsLocal) {
    Route localRoute(1, juce::Uuid::null(), "Local", "input");
    Route remoteRoute(2, remoteNode1, "Remote", "output");

    EXPECT_TRUE(localRoute.isLocal());
    EXPECT_FALSE(remoteRoute.isLocal());
}

// Test concurrent route addition
TEST_F(RoutingTableTest, HandlesConcurrentAddition) {
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            table->addRoute(i, remoteNode1, "Device " + juce::String(i), "input");
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
        table->addRoute(i, remoteNode1, "Device " + juce::String(i), "input");
    }

    // Remove concurrently
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i]() {
            table->removeRoute(i);
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
            table->addRoute(id++, remoteNode1, "Device", "input");
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
    auto route = table->getRoute(999);

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
    table->addRoute(1, remoteNode1, "Device 1", "input");
    table->addRoute(2, remoteNode1, "Device 2", "output");

    std::vector<Route> emptyRoutes;
    table->replaceNodeRoutes(remoteNode1, emptyRoutes);

    EXPECT_EQ(0, table->getNodeRouteCount(remoteNode1));
}

// Test local node identification
TEST_F(RoutingTableTest, IdentifiesLocalNode) {
    Route route(1, juce::Uuid::null(), "Device", "input");

    EXPECT_TRUE(route.nodeId.isNull());
    EXPECT_TRUE(route.isLocal());
}

// Test device type preservation
TEST_F(RoutingTableTest, PreservesDeviceType) {
    table->addRoute(1, localNode, "Input Device", "input");
    table->addRoute(2, localNode, "Output Device", "output");

    auto inputRoute = table->getRoute(1);
    auto outputRoute = table->getRoute(2);

    ASSERT_TRUE(inputRoute.has_value());
    ASSERT_TRUE(outputRoute.has_value());
    EXPECT_EQ("input", inputRoute->deviceType);
    EXPECT_EQ("output", outputRoute->deviceType);
}

// Test node routes for non-existent node
TEST_F(RoutingTableTest, GetsEmptyNodeRoutes) {
    juce::Uuid nonExistentNode;

    auto routes = table->getNodeRoutes(nonExistentNode);

    EXPECT_TRUE(routes.empty());
}

// Test mixed local and remote routes
TEST_F(RoutingTableTest, HandlesMixedRoutes) {
    table->addRoute(1, localNode, "Local", "input");
    table->addRoute(2, remoteNode1, "Remote", "output");

    EXPECT_EQ(2, table->getTotalRouteCount());
    EXPECT_EQ(1, table->getLocalRouteCount());
    EXPECT_EQ(1, table->getRemoteRouteCount());
}
