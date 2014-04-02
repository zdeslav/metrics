Running the server      
==================

If you don't want to run a separate `statsd` server, you can run it locally, in
the same process as your client. Just configure it and run it:

~~~{.cpp}
metrics::server start_local_server(unsigned int port)
{
    // following code sets up an in-process server which outputs data
    // to console and a file.
    // before each flush, on_flush method is called
    auto on_flush = [] { printf("flushing!"); };
    console_backend console;
    file_backend file("d:\\dev\\metrics\\statsd.data");

    auto cfg = metrics::server_config(port)
        .pre_flush(on_flush)      // callback can be used for custom metrics, etc
        .flush_every(10)          // flush measurements every 10 seconds
        .add_backend(console)     // send data to console for display
        .add_backend(file)        // and also to file
        .log("console");          // log metrics to console

    // returned server instance is just a proxy to the server thread, so it is
    // safe to share it around
    return server::run(cfg);
}
~~~