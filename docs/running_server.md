Running the server      
==================

If you don't want to run a separate `statsd` server, you can run it locally, in
the same process as your client. Just configure it and run it:

~~~{.cpp}
    // set up an inproc server 
    auto cfg = server_config(12345)      // listening on port 12345
               .flush_every(30)          // flush measurements every 30 seconds
               .add_backend("graphite"); // send data to graphite for display;    

    server::run(cfg);
~~~

