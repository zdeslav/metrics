Metric++
========

Metric++ is a C++ library which provides simple metrics tracking for Windows
applications, inspired by [statsd] metrics.

It is based on [statsd protocol], and provides C++ client library for statsd,
as well as server implementation.

The collected metrics can be sent to various backends to analyze and process
data. E.g. they can be displayed in [graphite] or one of its numerous
[dashboards](http://dashboarddude.com/blog/2013/01/23/dashboards-for-graphite):

![graphite showing metrics](docs/img/stats-graph.png)

[statsd]: https://github.com/etsy/statsd/blob/master/docs/metric_types.md/ "statsd metrics"
[statsd protocol]: https://github.com/b/statsd_spec
[graphite]: http://graphite.wikidot.com/


Building the software
---------------------

Open the `metrics.sln` solution in Visual Studio 2010 or 2013,
choose the appropriate configuration and build it - that's it.

For more details, see [here](docs/how_to_build.md)

### Documentation

Metric++ is thoroughly documented.

To generate the full doxygen documentation for the library, execute `build_docs.bat`
script in the project's root folder. The documentation will be generated in
directory `build\docs\html`, just open `index.html` file.

Usage
-----

Client application sends metrics to server, which aggregates them and sends them to
various backends. Here's more info on [how it works](docs/how_it_works.md).

### Setting up the client

Before writing any metrics, you need to set up the client:

~~~{.cpp}
metrics::setup_client("localhost", 9999); // point client to the server
~~~

That's it, you are ready to start tracking the metrics (assuming that the server
is running). Of course, [additional client settings](docs/setting_up_client.md) 
can be specified.

The server can easily be hosted in the same process. For more details check
'[Running the server](docs/running_server.md)'.

### Writing metrics

The library supports [3 types of metrics](docs/metric_types.md): counters, timers and gauges.

#### Timers

~~~{.cpp}
// setting a timer
void test_fn_0()
{
    int time = get_time_ms();
    // do something lengthy
    metrics::measure("app.fn.duration", get_time_ms() - time);
}

// simpler:
void test_fn_1()
{
    metrics::auto_timer _("app.fn.duration");
    // do something lengthy
}   // <- "app.fn.duration" stored here when auto_timer destructor is called

// even simpler:
void test_fn_2()
{
    MEASURE_FN();
    // do something lengthy
}   // <- "app.fn.test_fn_2" timer stored here

~~~

#### Counters

~~~{.cpp}
void login(const char* user)
{
    metrics::inc("app.logins");           // increment login counter.
    if(!try_login(user))                  // in case of login error...
    {
       metrics::inc("app.logins.failed"); // ...increment counter of failed logins
       ...
    }
}
~~~


Topics
------

* [How it works](docs/how_it_works.md)
* [Types of metrics](docs/metric_types.md)
* [Running the server](docs/running_server.md)
* [Protocol](docs/protocol.md)
