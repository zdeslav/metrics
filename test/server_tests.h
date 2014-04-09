#pragma once

#include "gtest/gtest.h"
#include "../metrics/metrics_server.h"

namespace metrics
{
    // these functions are both declared and defined in metrics_server.cpp,
    // therefore we need to provide declarations to make compiler happy
    timer_data process_timer(const std::string& name, const std::vector<int>& values);
    stats flush_metrics(const storage& storage, unsigned int period_ms);
    void process_metric(storage* storage, char* buff, size_t len);
}

TEST(ServerTest, TimerFlush) {
    std::vector<int> values = { 17, 13, 15, 16, 18 };
    metrics::timer_data data = metrics::process_timer("test.timer", values);

    EXPECT_EQ("test.timer", data.metric);
    EXPECT_DOUBLE_EQ(15.8, data.avg);
    EXPECT_EQ(13, data.min);
    EXPECT_EQ(18, data.max);
    EXPECT_EQ(5, data.count);
    EXPECT_EQ(79, data.sum);
    EXPECT_NEAR(1.72047, data.stddev, 0.00001);
}

TEST(ServerTest, FlushLogic) {
    metrics::storage store;
    auto before = metrics::timer::now();
    auto stats = metrics::flush_metrics(store, 10000);
    auto after = metrics::timer::now();
    EXPECT_EQ(0, stats.counters.size());
    EXPECT_EQ(0, stats.gauges.size());
    EXPECT_EQ(0, stats.timers.size());
    EXPECT_GE(stats.timestamp, before);
    EXPECT_LE(stats.timestamp, after);
}

TEST(ServerTest, TimerProcessing) {
    metrics::storage store;

    char metric1[] = "stats.test.timer:25|ms";  
    auto before = metrics::timer::now();
    process_metric(&store, metric1, strlen(metric1));
    auto after = metrics::timer::now();

    EXPECT_EQ(1, store.counters.size());
    EXPECT_EQ(1, store.counters[metrics::builtin::internal_metrics_count]);

    auto ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(1, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);         

    EXPECT_EQ(1, store.timers.size());
    EXPECT_EQ(1, store.timers["stats.test.timer"].size());
    EXPECT_EQ(25, store.timers["stats.test.timer"].back());

    char metric2[] = "stats.test.timer:100|ms";  
    before = metrics::timer::now();
    process_metric(&store, metric2, strlen(metric2));
    after = metrics::timer::now();

    EXPECT_EQ(1, store.counters.size());
    EXPECT_EQ(2, store.counters[metrics::builtin::internal_metrics_count]);

    ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(1, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);

    EXPECT_EQ(1, store.timers.size());
    EXPECT_EQ(2, store.timers["stats.test.timer"].size());
    EXPECT_EQ(100, store.timers["stats.test.timer"].back());  
}

TEST(ServerTest, GaugeProcessing) {
    metrics::storage store;

    char metric1[] = "stats.test.gauge:5|g";
    auto before = metrics::timer::now();
    process_metric(&store, metric1, strlen(metric1));
    auto after = metrics::timer::now();

    EXPECT_EQ(1, store.counters.size());
    EXPECT_EQ(1, store.counters[metrics::builtin::internal_metrics_count]);

    auto ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(2, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);
    EXPECT_EQ(5, store.gauges["stats.test.gauge"]);

    EXPECT_EQ(0, store.timers.size());

    char metric2[] = "stats.test.gauge:10|g";
    before = metrics::timer::now();
    process_metric(&store, metric2, strlen(metric2));
    after = metrics::timer::now();

    EXPECT_EQ(1, store.counters.size());
    EXPECT_EQ(2, store.counters[metrics::builtin::internal_metrics_count]);

    ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(2, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);
    EXPECT_EQ(10, store.gauges["stats.test.gauge"]);
    
    EXPECT_EQ(0, store.timers.size());
}

TEST(ServerTest, CounterProcessing) {
    metrics::storage store;

    char metric1[] = "stats.test.counter:1|c";
    auto before = metrics::timer::now();
    process_metric(&store, metric1, strlen(metric1));
    auto after = metrics::timer::now();

    EXPECT_EQ(2, store.counters.size());
    EXPECT_EQ(1, store.counters[metrics::builtin::internal_metrics_count]);
    EXPECT_EQ(1, store.counters["stats.test.counter"]);

    auto ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(1, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);

    EXPECT_EQ(0, store.timers.size());

    char metric2[] = "stats.test.counter:3|c";
    before = metrics::timer::now();
    process_metric(&store, metric2, strlen(metric2));
    after = metrics::timer::now();

    EXPECT_EQ(2, store.counters.size());
    EXPECT_EQ(2, store.counters[metrics::builtin::internal_metrics_count]);
    EXPECT_EQ(4, store.counters["stats.test.counter"]);

    ts = store.gauges[metrics::builtin::internal_metrics_last_seen];
    EXPECT_EQ(1, store.gauges.size());
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);

    EXPECT_EQ(0, store.timers.size());
}