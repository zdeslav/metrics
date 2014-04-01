#pragma once

#include <map>
#include <vector>

namespace metrics
{
    /// prototype for callback function to be called immediately before flush.
    typedef void(*FLUSH_CALLBACK)(void);

    class server_config;
    class backend;

    /// Handles settings for local server instance.
    class server_config
    {
        unsigned int m_flush_period;
        unsigned int m_port;
        FLUSH_CALLBACK m_callback;
        std::vector<backend*> m_backends;

    public:
        /**
        * Creates a metrics::server_config instance.
        * Configuration object is used to specify the settings for the local server
        * instance, which then runs in the same process, after server::run() is called.
        * This function will try to start winsock (WSAStartup) if it finds that 
        * winsock is not already started.
        *
        * @param port The port on which server is listening. The default is 9999.
        * @throws config_exception Thrown if invalid config parameters are 
        *         specified or if automatic winsock startup fails.
        *
        * Example:
        * ~~~{.cpp}
        * // set up an inproc server 
        * metrics::server start_local_server(unsigned int port)
        * {
        *     auto on_flush = [] { dbg_print("flushing!"); };
        *     auto console = new console_backend();
        *     auto file = new file_backend("d:\\dev\\metrics\\statsd.data");
        *
        *     auto cfg = metrics::server_config(port)
        *         .pre_flush(on_flush)      // can be used for custom metrics, etc
        *         .flush_every(10)          // flush measurements every 10 seconds
        *         .add_backend(console)     // send data to console for display
        *         .add_backend(file)        // send data to file
        *         .log("console");          // log metrics to console
        *
        *     return server::run(cfg);
        * }
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
        * @param backend An instance of a backend.
        */
        server_config& add_backend(backend* backend);

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
        std::vector<backend*> backends() const { return m_backends; }
    };

    /// Represents a instance of the server.
    class server
    {
        server_config m_cfg;
        server(const server_config& cfg) : m_cfg(cfg) { ; }

    public:
        /**
        * starts the server, based on specified configuration
        * @param cfg Setting used by the server
        */
        static server run(const server_config& cfg);

        /**
        * notifies the server to stop processing incoming messages. If server
        * was configured to create a thread, the thread will exit.
        */
        void stop();

        const server_config& config() const { return m_cfg; }
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


    struct timer_data
    {
        std::string metric;
        int count;
        int max;
        int min;
        long long sum;
        double avg;
        double stddev;

        std::string dump()
        {
            char txt[256];
            _snprintf_s(txt, _countof(txt), _TRUNCATE, 
                "%s - cnt: %d, min: %d, max: %d, sum: %lld, avg: %.2f, stddev: %.2f",
                metric.c_str(), count, min, max, sum, avg, stddev);
            return txt;
        }
    };

    /// contains processed metric statistics
    struct stats
    {
        timer::time_point timestamp;
        std::map<std::string, double> counters; ///< counter data
        std::map<std::string, int> gauges; ///< gauge data
        std::map<std::string, timer_data> timers; ///< timer data
    };

    /**
    * Generic interface which must be implemented by all backends.
    * Backends are registered with server_config::add_backend().
    */
    class backend
    {
    public:
        virtual ~backend(){}

        /**
        * Must be implemented by concrete backend implementation.
        * @param stats Contains processed metric statistics. Each backend
        *              decides how to handle the data.
        */
        virtual void process_stats(const stats& stats) = 0;
    };

    /// Simple backend to dump stats to console
    class console_backend : public backend
    {
    public:
        virtual void process_stats(const stats& stats);
    };

    /// Simple backend to dump stats to file
    class file_backend : public backend
    {
        std::string m_filename;
    public:
        file_backend(const char* filename) : m_filename(filename){ ; }
        virtual void process_stats(const stats& stats);
    };

    /*
    class event_log_backend : public backend
    {
    public:
        event_log_backend();
        virtual void process_stats(const stats& stats);
    };

    class graphite_backend : public backend
    {
    public:
        graphite_backend(const char* host, int port);
        virtual void process_stats(const stats& stats);
    };

    class syslog_udp_backend : public backend
    {
    public:
        syslog_udp_backend(const char* host, int port);
        virtual void process_stats(const stats& stats);
    };

    class syslog_file_backend : public backend
    {
        std::string m_filename;
    public:
        syslog_udp_backend(const char* filename) : m_filename(filename){ ; }
        virtual void process_stats(const stats& stats);
    };
    */
}

