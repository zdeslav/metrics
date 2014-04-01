#include "stdafx.h"
#include "metrics.h"
#include "metrics_server.h"
#include "windows.h"

namespace metrics
{
    storage g_storage;

    server_config::server_config(unsigned int port) :
        m_port(port),
        m_flush_period(60),
        m_callback([]{}) // NOP callback
    {
        ensure_winsock_started();
    }

    server_config& server_config::flush_every(unsigned int period) {
        m_flush_period = period;
        return *this;
    }

    server_config& server_config::add_backend(const char* back) {
        // todo
        return *this;
    }

    server_config& server_config::pre_flush(FLUSH_CALLBACK callback) {
        m_callback = callback;
        return *this;
    }

    server_config& server_config::log(const char* target) {
        // todo
        return *this;
    }

    struct timer_data
    {
        std::string metric;
        int count;
        int max;
        int min;
        long long sum;
        double avg;
        double stddev;

        void dump()
        {
            dbg_print("metric %s: cnt = %d, min = %d, max = %d, sum = %llu, avg = %g, stddev = %g\n",
                      metric.c_str(), count, min, max, sum, avg, stddev);
        }
    };

    timer_data process_timer(const std::string& name, std::vector<int> values)
    {
        timer_data data = { name, values.size(), 0, 0, 0, 0, 0 };   
        if (data.count == 0) return data;

        data.min = values[0];
        data.max = data.min;
        long long square_sum = 0;
        for (auto v : values)
        {
            if (v > data.max) data.max = v;
            if (v < data.min) data.min = v;
            data.sum += v;
            square_sum += v * v;
        }

        data.avg = data.sum / (double)data.count;
        double var = square_sum / (double)data.count - data.avg * data.avg;
        data.stddev = sqrt(var);
        return data;
    }

    void flush_metrics(unsigned int period_ms)
    {
        auto time = clock();
        dbg_print("processing storage: C[%d], G[%d], H[%d]\n",
                  g_storage.counters.size(), 
                  g_storage.gauges.size(), 
                  g_storage.timers.size());

        auto period =  period_ms / (double)CLOCKS_PER_SEC;

        for (auto c : g_storage.counters)
        {
            dbg_print("  C: %s - %g  1/s\n", c.first.c_str(), c.second / period);
        }
        for (auto g : g_storage.gauges)
        {
            dbg_print("  G: %s - %d  1/s\n", g.first.c_str(), g.second);
        }
        for (auto g : g_storage.timers)
        {
            auto data = process_timer(g.first, g.second);
            dbg_print("  H: ");
            data.dump();
        }

        g_storage.clear();
        time = clock() - time;
        printf("flushing took %d ms", time);
    }

    void process_metric(char* buff, size_t len)
    {
        auto pipe_pos = strrchr(buff, '|');
        auto colon_pos = strrchr(buff, ':');
        if (!colon_pos || !pipe_pos) {
            dbg_print("unknown metric: %s\n", buff);
            return;
        }

        metric_type metric;
        if (strcmp(pipe_pos, "|h") == 0) metric = histogram;
        else if (strcmp(pipe_pos, "|g") == 0) metric = gauge;
        else if (strcmp(pipe_pos, "|c") == 0) metric = counter;
        else if (strcmp(pipe_pos, "|ms") == 0) metric = histogram;
        else {
            dbg_print("unknown metric type: %s\n", pipe_pos);
            return;
        }

        *pipe_pos = '\0';
        *colon_pos = '\0';
        colon_pos++;
        std::string metric_name = buff;
        int value = atol(colon_pos);

        dbg_print("storing metric %d: %s [%d]\n", metric, metric_name.c_str(), value);

        switch (metric)
        {
            case metrics::counter:
                g_storage.counters[metric_name] += value;
                break;
            case metrics::gauge:
                g_storage.gauges[metric_name] = value;
                break;
            case metrics::histogram:
                g_storage.timers[metric_name].push_back(value);
                break;
        }
    }

    DWORD WINAPI ThreadProc(LPVOID params)
    {     
        const int BUFSIZE = 4096;

        server_config* cfg = static_cast<server_config*>(params);
        auto port = cfg->port();
        auto period_ms = cfg->flush_period() * CLOCKS_PER_SEC;
        auto flush_cb = cfg->callback();
        delete cfg;

        struct sockaddr_in myaddr;      // our address 
        struct sockaddr_in remaddr;     // remote address 
        int addrlen = sizeof(remaddr);  // length of addresses 
        int recvlen;                    // # bytes received 
        int fd;                         // our socket 
        char buf[BUFSIZE];              // receive buffer 

        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // create a UDP socket
            dbg_print("cannot create server socket: error: %d\n", WSAGetLastError());
            return 1;
        }

        auto time = clock();
        /* bind the socket to any valid IP address and a specific port */
        memset((char *)&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
            dbg_print("bind failed");
            return 1;
        }

        dbg_print("inproc server listening at port %d\n", port);

        
        int maxfd = fd;
        fd_set static_rdset, rdset;
        timeval timeout = { 1, 0 };

        FD_ZERO(&static_rdset);
        FD_SET(fd, &static_rdset);

        while (true) {
            rdset = static_rdset;
            select(maxfd + 1, &rdset, NULL, NULL, &timeout);

            if (FD_ISSET(fd, &rdset)) {
                recvlen = recvfrom(fd, buf, BUFSIZE, 0, (sockaddr*)&remaddr, &addrlen);
                if (recvlen > 0) {
                    buf[recvlen] = 0;
                    if (strcmp(buf, "stop") == 0) {
                        dbg_print(" > received STOP cmd, stopping server\n");
                        return 0;
                    }
                    dbg_print(" > received:%s (%d bytes)\n", buf, recvlen);
                    process_metric(buf, recvlen);
                }                 
            }

            auto ellapsed = clock() - time;
            if (ellapsed >= period_ms)
            {
                time = clock();
                dbg_print(" > flushing metrics at %d...\n", time);
                flush_cb();
                flush_metrics(period_ms);
            }
        }
    }

    server server::run(const server_config& cfg)
    {
        dbg_print("starting inproc server....\n");
        DWORD thread_id;
        CreateThread(NULL, 0, ThreadProc, new server_config(cfg), 0, &thread_id);
        return server(cfg);
    }

    server::server(const server_config& cfg) : m_cfg(cfg) {;}
    void server::stop()
    {
        dbg_print("sending stop cmd...\n");
        const char* cmd = "stop";
        send_to_server(cmd, strlen(cmd));
    }

}