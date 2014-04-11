Setting up client
=================

As noted earlies, before writing any metrics, you need to set up the client:

~~~{.cpp}
    // this is the simplest client setup
    metrics::setup_client("localhost"); // point client to the server, default port 9999
~~~

That's it, you are ready to start tracking the metrics (assuming that server is
running). 

There are also some other useful client settings, here's a more complex setup:

~~~{.cpp}
    // setup the client, only host and port settings are required
    metrics::setup_client("localhost", 5423)  // point client to the server
        .set_debug(true)                      // turn on debug tracing
        .set_namespace("myapp")               // specify metrics namespace, default is "stats"
        .track_default_metrics(metrics::all); // track default system and process metrics
~~~

Now you can start tracking the metrics.

NOTES:

* `set_debug(true)` turns on debug tracing, which writes the messages to console.
  This is useful for quick troubleshooting session.
* `set_namespace` replaces the default `stats` root in metric name with another one.
* `track_default_metrics` functionality is not yet implemented.
