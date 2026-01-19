#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <memory>
#include <coev/coev.h>
#include <cstdint>
#include <chrono>
#include <functional>

#include "broker.h"
#include "config.h"
#include "access_token.h"
#include "error.h"

// Start with basic tests
TEST(BrokerTest, BasicConstruction)
{
    // Test basic broker construction using shared_ptr
    auto broker = std::make_shared<Broker>("abc:123");

    // Just verify that the broker was constructed successfully
    EXPECT_TRUE(broker != nullptr);

    // Test basic accessors that don't call any other methods
    EXPECT_EQ(broker->ID(), -1);
    EXPECT_EQ(broker->Addr(), "abc:123");
}

TEST(BrokerTest, DifferentConstructors)
{
    // Test all three constructor variations
    auto broker1 = std::make_shared<Broker>(); // Default constructor
    // Default constructor uses default initialization, so m_id is 0
    EXPECT_EQ(broker1->ID(), 0);
    EXPECT_EQ(broker1->Addr(), "");

    auto broker2 = std::make_shared<Broker>("localhost:9092"); // Constructor with address
    EXPECT_EQ(broker2->ID(), -1);
    EXPECT_EQ(broker2->Addr(), "localhost:9092");

    auto broker3 = std::make_shared<Broker>(5, "localhost:9093"); // Constructor with ID and address
    EXPECT_EQ(broker3->ID(), 5);
    EXPECT_EQ(broker3->Addr(), "localhost:9093");
}

TEST(BrokerTest, InitialState)
{
    // Test initial state of broker members
    auto broker = std::make_shared<Broker>("localhost:9092");

    // m_conn is a Connect object, which is initially valid but not opened
    EXPECT_TRUE(broker->m_conn.IsClosed());   // Connect object starts in CLOSED state
    EXPECT_FALSE(broker->m_conn.IsOpening()); // Not opening initially
    EXPECT_FALSE(broker->m_conn.IsOpened());  // Not opened yet
    EXPECT_EQ(broker->m_id, -1);
    EXPECT_EQ(broker->m_correlation_id, 0);
    EXPECT_EQ(broker->m_session_reauthentication_time, 0);
}

TEST(BrokerTest, Accessors)
{
    // Test basic broker accessors
    auto broker = std::make_shared<Broker>("abc:123");

    // New broker should have default values
    EXPECT_EQ(broker->ID(), -1);
    EXPECT_EQ(broker->Addr(), "abc:123");
    EXPECT_EQ(broker->Rack(), "");

    // Test setting ID directly
    broker->m_id = 34;
    EXPECT_EQ(broker->ID(), 34);

    // Test setting rack directly
    broker->m_rack = "dc1";
    EXPECT_EQ(broker->Rack(), "dc1");
}

TEST(BrokerTest, ConnectedState)
{
    // Test broker connected state
    auto broker = std::make_shared<Broker>("localhost:9092");

    // A newly constructed broker should not be connected
    // Connected() returns m_conn, which is initially true (Connect object is valid)
    // So we can't use Connected() to check if the broker is actually connected
    // Instead, we can check if it's opened
    EXPECT_FALSE(broker->m_conn.IsOpened());

    // TLSConnectionState returns an int, which is 0 when not connected
    EXPECT_EQ(broker->TLSConnectionState(), 0);
}

TEST(BrokerTest, ApiVersionsMap)
{
    // Test API versions map functionality
    auto broker = std::make_shared<Broker>("localhost:9092");

    // Initial API versions map should be empty
    EXPECT_TRUE(broker->m_broker_api_versions.empty());

    // Test adding API versions
    broker->m_broker_api_versions[0] = {0, 1};
    broker->m_broker_api_versions[1] = {0, 2};

    EXPECT_EQ(broker->m_broker_api_versions.size(), 2);

    // Test accessing API versions - compare individual members instead of using == operator
    auto &range0 = broker->m_broker_api_versions[0];
    EXPECT_EQ(range0.m_min_version, 0);
    EXPECT_EQ(range0.m_max_version, 1);

    auto &range1 = broker->m_broker_api_versions[1];
    EXPECT_EQ(range1.m_min_version, 0);
    EXPECT_EQ(range1.m_max_version, 2);
}

TEST(BrokerTest, MultipleBrokers)
{
    // Test multiple brokers with different properties
    auto broker1 = std::make_shared<Broker>(1, "localhost:9092");
    auto broker2 = std::make_shared<Broker>(2, "localhost:9093");
    auto broker3 = std::make_shared<Broker>(3, "localhost:9094");

    // Set different racks
    broker1->m_rack = "dc1";
    broker2->m_rack = "dc2";
    broker3->m_rack = "dc3";

    // Check that all brokers have distinct properties
    EXPECT_NE(broker1->ID(), broker2->ID());
    EXPECT_NE(broker2->ID(), broker3->ID());
    EXPECT_NE(broker1->Addr(), broker2->Addr());
    EXPECT_NE(broker2->Addr(), broker3->Addr());
    EXPECT_NE(broker1->Rack(), broker2->Rack());
    EXPECT_NE(broker2->Rack(), broker3->Rack());
}

TEST(BrokerTest, CorrelationID)
{
    // Test broker correlation ID generation
    auto broker = std::make_shared<Broker>("localhost:9092");

    // Initial correlation ID should be 0
    EXPECT_EQ(broker->m_correlation_id, 0);

    // Simulate incrementing correlation ID
    broker->m_correlation_id++;
    EXPECT_EQ(broker->m_correlation_id, 1);

    broker->m_correlation_id += 100;
    EXPECT_EQ(broker->m_correlation_id, 101);
}

TEST(BrokerTest, SessionReauthenticationTime)
{
    // Test broker session reauthentication time
    auto broker = std::make_shared<Broker>("localhost:9092");

    // Initial reauthentication time should be 0
    EXPECT_EQ(broker->m_session_reauthentication_time, 0);

    // Test updating reauthentication time
    broker->m_session_reauthentication_time = 1234567890;
    EXPECT_EQ(broker->m_session_reauthentication_time, 1234567890);

    broker->m_session_reauthentication_time += 3600000;             // Add 1 hour in milliseconds
    EXPECT_EQ(broker->m_session_reauthentication_time, 1238167890); // Correct expected value
}
