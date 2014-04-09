Running the server      
==================

If you don't want to run a separate `statsd` server, you can run it locally, in
the same process as your client. Just configure it and run it:

~~~{.cpp}
    // following code is the simplest way to start a server. Just specify some
    // backend and run it on default port:
    
    auto cfg = metrics::server_config()               // use default port 9999
        .add_backend(file_backend("d:\\stats.log"));  // write stats to file
    
    server::run(cfg);
~~~

Of course, additional settings can be specified:

~~~{.cpp}
metrics::server start_local_server(unsigned int port)
{
    // following code sets up an in-process server which outputs data to console and a file.
    
    // you can specify a function to be called before each flush, 
    // just register it with server_config::pre_flush()
    auto on_flush = [] { printf("flushing!"); };

    // you can also specify a function to be called when server is started or stopped: 
    auto on_server_info = [] (server_events e) { printf("server event:", e); };

    // here we used console and file backends
    console_backend console;
    file_backend file("d:\\stats.log");

    auto cfg = metrics::server_config(port)
        .pre_flush(on_flush)      // callback can be used for custom metrics, etc
        .flush_every(10)          // flush measurements every 10 seconds
        .add_backend(console)     // send data to console for display
        .add_backend(file)        // and also to file
        .add_server_listener(on_server_info); // register for server notifications

    // returned server instance is just a proxy to the server thread, so it is
    // safe to share it around. You can call stop() on it, to stop server gracefully
    return server::run(cfg);
}
~~~
