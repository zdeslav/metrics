Running the server      
==================

If you don't want to run a separate `statsd` server, you can run it locally, in
the same process as your client. Just configure it and run it:

~~~{.cpp}
metrics::server start_local_server(unsigned int port)
{
    auto on_flush = [] { dbg_print("flushing!"); };
    auto console = new console_backend();
    auto file = new file_backend("d:\\dev\\metrics\\statsd.data");

    auto cfg = metrics::server_config(port)
        .pre_flush(on_flush)      // can be used for custom metrics, etc
        .flush_every(10)          // flush measurements every 10 seconds
        .add_backend(console)     // send data to console for display
        .add_backend(file)        // send data to file
        .log("console");          // log metrics to console

    // returned server instance is just a proxy to the server thread, so it is
    // safe to share it around
    return server::run(cfg);
}
~~~

