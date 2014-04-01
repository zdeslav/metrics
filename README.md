Metrics             
=======

*UNDER CONSTRUCTION*

Metrics is a C++ library which provides simple metrics tracking for Windows 
applications, inspired by [statsd] metrics.

It is based on [statsd protocol], and provides C++ client library for statsd 
server, as well as server implementation.

[statsd]: https://github.com/etsy/statsd/blob/master/docs/metric_types.md/ "statsd metrics"
[statsd protocol]: https://github.com/b/statsd_spec


Documentation
-------------

To generate the full doxygen documentation for the library, execute `build_docs.bat`
script in the project's root folder. The documentation will be generated in
directory `build\docs\html`, just open `index.html` file.

Usage
-----

### Setting up the client

Before writing any metrics, you need to set up the client:

~~~{.cpp}
    // setup the client
    metrics::setup_client("127.0.0.1", 9999) // point client to the server
        .set_debug(true)                     // turn on debug tracing
        .set_namespace("myapp")              // specify namespace, default is "stats"
        .track_default_metrics();            // track default system and process metrics
~~~

Now you can start tracking the metrics.

The server can easily be hosted in the same process. For more details check
[Running the server](docs/running_server.md).

### Timers

~~~{.cpp}
// setting a timer
void test_fn_0()
{
    auto time = clock();
    // do something lengthy
    metrics::measure("app.fn.duration", clock() - time);
}

// simpler:
void test_fn_1()
{
    metrics::auto_timer _("app.fn.duration");  
    // do something lengthy
}   // <- "app.fn.duration" store here when auto_timer destructor is called

// even simpler:
void test_fn_2()
{
    MEASURE_FN(); 
    // do something lengthy
}   // <- "app.fn.test_fn_2" timer stored here

~~~

### Counters

~~~{.cpp}
void login(const char* user)
{
    metrics::increment("app.logins");
    if(!login(user))
    {
       metrics::increment("app.logins.failed");
       ...
    }
}   
~~~


Topics
------

* [Overview](docs/overview.md)
    * [How it works](docs/how_it_works.md)
    * [Types of metrics](docs/metric_types.md)
    * [Running the server](docs/running_server.md)
    * [Protocol](docs/protocol.md)
