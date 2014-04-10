Running the server      
==================

If you don't want to run a separate `statsd` server, you can run the server 
provided by metric++ locally, in the same process as your client. 

Just configure it and run it:

~~~{.cpp}
    // following code is the simplest way to start a server. Just specify some
    // backend and run it on default port:
    
    auto cfg = metrics::server_config()               // use default port 9999
        .add_backend(file_backend("d:\\stats.log"));  // write stats to file
    
    server::run(cfg);
~~~

The server receives the metrics from the client and flushes them periodically
(this is controlled by `metrics::server_config::flush_every` setting). Flushing 
procedure calculates metrics data (e.g. for counters it calculates number of 
entries per second, for timers it calculates min, max, sum, count, average and 
standard deviation), and sends it to all the specify backends.

Backend is any C++ entity that looks like function which takes `const metrics::stats&`,
so it can be a function, an object, a lambda, etc. For more details, check
`metrics::server_config::add_backend` method.

Of course, additional settings can also be specified. Here's an example of
setting up a more complex server:

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
