How it works
============

There are three parts to metrics tracking with metric++:

* client - it sends raw metrics to the server. Examples:
    * every time a user logs in, `app.login` counter is incremented
    * duration of each request is measured as `app.request.duration` timer
* server - it receives metrics from client and periodically (e.g. every 60s)
    flushes them. In examples above:
    * `app.login` total will be divided by flush period to calculate number of
      logins per second
    * `app.request.duration` is processed to calculate min, max, average, sum,
      count and standard deviation.
* backend - processed metrics are sent to backend. It can be a console, a web
    app with graphical representation, a database or something completely
    different.


Three types of measurements are supported:

* counters - counts the occurrences
* timers - measures the durations of events
* gauges - arbitrary values, which are not processed

For more details, see [statsd](https://github.com/etsy/statsd/blob/master/docs/metric_types.md/)

Metric++ provides tools for client and server part of the story:

* by including `metrics.h` you can use functions like `metrics::inc`,
  `metrics::measure`, `metrics::set` or classes like `metrics::auto_timer` to
  send metrics to server. The wire representation is compatible with statsd
  so if you have a running statsd instance, just
  [point the client](docs/setting_up_client.md) to it.
* by including `metrics_server.h` you get everything you need to [turn your app 
  into metrics server](docs/running_server.md).

Please take a look at provided metrics application and unit tests for detailed
info on usage.