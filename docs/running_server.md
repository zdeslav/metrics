Running the server      
==================

If you don't want to run a separate `statsd` server, you can run it locally, in
the same process as your client. Just configure it and run it:

~~~{.cpp}
    // set up an inproc server listening on port 12345
    metrics::server svr = metrics::local_server(12345)
        .own_thread()             // run it on another thread
        .pre_flush(on_flush)      // can be used for custom metrics, etc
        .flush_each(30)           // flush measurements every 30 seconds
        .add_backend("graphite")  // send data to graphite for display
        .log("console")           // log metrics to console
        .run();                   // start the server
~~~

