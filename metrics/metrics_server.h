#pragma once

#include <map>
#include <vector>

namespace metrics
{
    /// prototype for callback function to be called immediately before flush.
    typedef void(*FLUSH_CALLBACK)(void);

    class server_config;

    /// Handles settings for local server instance.
    class server_config
    {
        unsigned int m_flush_period;
        unsigned int m_port;
        FLUSH_CALLBACK m_callback;

    public:
        /**
        * Creates a metrics::server_config instance.
        * Configuration object is used to specify the settings for the local server
        * instance, which then runs in the same process, after server::run() is called.
        *
        * @param port The port on which server is listening. The default is 9999.
        *
        * Example:
        * ~~~{.cpp}
        * // set up an inproc server listening on port 12345
        * auto cfg = metrics::server_config(12345)
        *     .flush_every(30)          // flush measurements every 30 seconds
        *     .pre_flush(&on_flush)     // can be used for custom metrics, etc
        *     .add_backend("graphite")  // send data to graphite for display
        *     .log("console");          // log metrics to console
        * server::run(cfg);             // start the server
        * ~~~
        */

        server_config(unsigned int port = 9999);

        /**
        * Specifies the default flush period for server. The default is 60s.
        * @param period Flush period, in seconds.
        */
        server_config& flush_every(unsigned int period);

        /**
        * Tells the server to run on the same thread on which server::run() was 
        * called from. By default, server is running on another thread.
        */
        server_config& same_thread();

        /**
        * Adds a backend for flushed stats.
        * @param backend Name of the backend.
        * @todo *NOT IMPLEMENTED YET*
        */
        server_config& add_backend(const char* backend);

        /**
        * Specifies the callback function to be called before the values are
        * flushed. You can use this to add some metrics.
        * @param callback Callback function.
        *
        * Example:
        * ~~~{.cpp}
        * // set up an inproc server listening on port 12345
        * auto cfg = metrics::server_config(12345)
        *     .pre_flush([]{ printf("flushing..."); }); // use lambda
        * 
        * server::run(cfg);                             // start the server
        * ~~~
        */
        server_config& pre_flush(FLUSH_CALLBACK callback);

        /**
        * Specifies where the debug info will be logged.
        * @param target Output target: "console", ...
        * @todo *NOT IMPLEMENTED YET*
        */
        server_config& log(const char* target);

        unsigned int flush_period() const { return m_flush_period; }
        unsigned int port() const { return m_port; }
        FLUSH_CALLBACK callback() const { return m_callback; }
    };

    /// Represents a instance of the server.
    class server
    {
        server_config m_cfg;
        server(const server_config& cfg);
    public:
        static server run(const server_config& cfg);
        const server_config& config() const { return m_cfg; }
        void stop();
    };

    struct storage
    {
        std::map<std::string, unsigned int> counters;
        std::map<std::string, int> gauges;
        std::map<std::string, std::vector<int> > timers;

        void clear() {
            counters.clear();
            gauges.clear();
            timers.clear();
        }
    };

}

