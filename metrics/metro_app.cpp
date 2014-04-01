// metro_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "metrics.h"
#include "metrics_server.h"
#include "windows.h"

namespace metrics {
    METRIC_ID auth_req_count = "auth.requests.count";
    METRIC_ID auth_req_duration = "auth.requests.duration";
    METRIC_ID auth_req_total = "auth.requests.total";
    METRIC_ID auth_req_failed = "auth.requests.failed";
    METRIC_ID auth_req_queue_time = "auth.requests.queue_time";
    METRIC_ID auth_req_queue_depth = "auth.requests.queue_depth";

    METRIC_ID comm_req_iso = "comm.rq.iso";
    METRIC_ID comm_req_hiso = "comm.rq.hiso";
    METRIC_ID comm_req_ws = "comm.rq.ws";
    METRIC_ID comm_req_cics = "comm.rq.cics";

    METRIC_ID lengthy_op = "app.lengthy_op.duration";
};

using namespace metrics;

// todo:
// remove log('console') - make console a backend

void some_lengthy_function();
void check_timers(int iterations);

metrics::server start_local_server(unsigned int port)
{
    auto on_flush = [] { dbg_print("flushing!\n"); };

    auto cfg = metrics::server_config(port)
        .pre_flush(on_flush)      // can be used for custom metrics, etc
        .flush_each(10)           // flush measurements every 10 seconds
        .add_backend("graphite")  // send data to graphite for display
        .log("console");          // log metrics to console

    return server::run(cfg);
}

int _tmain(int argc, _TCHAR* argv[])
{
    // setup the client
    metrics::setup_client("127.0.0.1", 12345) // point client to the server
        .set_debug(true)                      // turn on debug tracing
        .set_namespace("myapp")               // specify namespace, default is "stats"
        .track_default_metrics();             // track default system and process metrics

    // set up an inproc server listening on port 12345, if you don't want to
    // run a separate server
    metrics::server svr = start_local_server(12345);

    some_lengthy_function();  // perform some lengthy operation and measure it

    check_timers(5);

    Sleep(15000); // wait for flush

    svr.stop();  // stop the server gracefully
	return 0;
}


void check_timers(int iterations)
{
    srand(GetTickCount());

    for (size_t i = 0; i < iterations; i++)
    {
        MEASURE_FN();
        int wait = 10 + rand() / 1000;
        Sleep(wait);
    }
}

void some_lengthy_function()
{
    metrics::auto_timer _(lengthy_op);  // measure function duration
    MEASURE_FN(); // measure function duration. metric name is set to 'app.fn.some_lengthy_function'

    // do something that takes time....
    // start tracking measurements
    metrics::increment(metrics::auth_req_count);              // counter
    metrics::measure(metrics::builtin::sys_cpu_load, 42);     // histogram/timer
    metrics::set(metrics::builtin::sys_disk_free_c, 1543);    // gauge

}