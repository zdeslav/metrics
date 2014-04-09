#pragma once

#include "gtest/gtest.h"
#include "../metrics/metrics.h"


TEST(ServerTest, CheckClientSettings) {
    ASSERT_THROW(metrics::setup_client(""), metrics::config_exception);
    ASSERT_THROW(metrics::setup_client("efgkleioger//*;/&g"), metrics::config_exception);
    ASSERT_NO_THROW(metrics::setup_client("127.0.0.1"));

    auto cfg = metrics::setup_client("127.0.0.1");

    ASSERT_THROW(cfg.track_default_metrics(0), metrics::config_exception);

    cfg.set_namespace("test_ns");
    EXPECT_STREQ("test_ns", cfg.get_namespace());

    cfg.set_debug(true);
    EXPECT_TRUE(cfg.is_debug());

    cfg.set_debug(false);
    EXPECT_FALSE(cfg.is_debug());

    
}