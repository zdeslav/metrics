#pragma once

#include "../metrics/metrics_server.h"
#include "gtest/gtest.h"

namespace metrics
{
    // these functions are both declared and defined in metrics_server.cpp,
    // therefore we need to provide declarations to make compiler happy
    timer_data process_timer(const std::string& name, const std::vector<int>& values);
    stats flush_metrics(const storage& storage, unsigned int period_ms);
    void process_metric(storage* storage, char* buff, size_t len);
}
metrics::server start(const metrics::server_config& cfg)
{
    auto svr = metrics::server::run(cfg);
    auto ts = metrics::timer::now();          
    // wait a bit until it spins up (hopefully)
    // todo: add a callback to denote that server has started/stopped
    while (metrics::timer::now() - ts < 100); 
    return svr;
}

void stop(metrics::server& svr)
{
    svr.stop();
    auto ts = metrics::timer::now();
    while (metrics::timer::now() - ts < 100); // wait a bit until it stops
}

TEST(ServerTest, TimerDataCalculation) {
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

TEST(ServerTest, FlushLogicForEmptyStor) {
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

/*
stats flush_metrics(const storage& storage, unsigned int period_ms)
{
stats stats;
stats.timestamp = timer::now();

auto period =  period_ms / 1000.0;

FOR_EACH (auto& c, storage.counters) stats.counters[c.first] = c.second / period;
FOR_EACH (auto& g, storage.gauges) stats.gauges[g.first] = g.second;
FOR_EACH (auto& t, storage.timers) stats.timers[t.first] = process_timer(t.first, t.second);

return stats; // todo: move
}

*/


bool operator == (const metrics::timer_data& lhs, const metrics::timer_data& rhs)
{
    return lhs.count == rhs.count
        && lhs.avg == rhs.avg
        && lhs.max == rhs.max
        && lhs.min == rhs.min
        && lhs.sum == rhs.sum
        && lhs.stddev == rhs.stddev
        && lhs.metric == rhs.metric;
}

TEST(ServerTest, Flushing) {
    metrics::storage store;

    store.counters["c.1"] = 5;
    store.counters["c.2"] = 10;

    store.gauges["g.1"] = 42;
    store.gauges["g.2"] = 999;

    store.timers["t.1"] = { 17, 13, 15, 16, 18 };
    store.timers["t.2"] = { 1, 2, 3};

    auto stats = metrics::flush_metrics(store, 10000);

    EXPECT_DOUBLE_EQ(0.5, stats.counters["c.1"]);
    EXPECT_DOUBLE_EQ(1, stats.counters["c.2"]);

    EXPECT_EQ(42, stats.gauges["g.1"]);
    EXPECT_EQ(999, stats.gauges["g.2"]);

    auto td1 = metrics::process_timer("t.1", { 17, 13, 15, 16, 18 } );
    auto td2 = metrics::process_timer("t.2", { 1, 2, 3 });

    EXPECT_TRUE(td1 == stats.timers["t.1"]);
    EXPECT_TRUE(td2 == stats.timers["t.2"]);
}

TEST(ServerTest, PreFlushCalled) {
    bool flush_called = false;
    auto cfg = metrics::server_config()
        .flush_every(1)
        .pre_flush([&](){ flush_called = true; });

    auto svr = start(cfg);
    auto ts = metrics::timer::now();
    while (!flush_called && metrics::timer::now() - ts < 3000);
    stop(svr);
    EXPECT_TRUE(flush_called);
}

TEST(ServerTest, CheckConfigSettings) {
    auto cfg = metrics::server_config();

    EXPECT_THROW(cfg.flush_every(0), metrics::config_exception);
    EXPECT_NO_THROW(cfg.flush_every(1));
    EXPECT_NO_THROW(cfg.flush_every(3600));
    EXPECT_THROW(cfg.flush_every(3601), metrics::config_exception);
}

TEST(ServerTest, NamespaceIsUsed) {
    std::string received_metric;

    auto backend = [&](const metrics::stats& s){ 
        if (!s.timers.empty()) received_metric = s.timers.cbegin()->first;
    };

    auto cfg = metrics::server_config().add_backend(backend).flush_every(1);
    auto svr = start(cfg);
    
    metrics::setup_client("127.0.0.1").set_namespace("xyz");
    metrics::measure("value", 1); // using timers to avoid filtering builtin counters/gauges

    auto ts = metrics::timer::now();          
    while (received_metric.empty() && metrics::timer::now() - ts < 2200);
    
    stop(svr);

    EXPECT_EQ("xyz.value", received_metric);
}

/*
TEST(ServerTest, NamespaceIsUsed) {
    std::string received_metric;

    auto backend = [&](const metrics::stats& s){ received_metric = s.timers.cbegin()->first; };
    auto svr = start([&](metrics::server_config& cfg) { cfg.add_backend(backend); });

    metrics::setup_client("127.0.0.1").set_namespace("xyz");
    metrics::measure("value", 1); // use timers to avoid filtering builtin counters/gauges

    stop_after_flush(svr, 2000);

    EXPECT_EQ("xyz.value", received_metric);
}  
*/

