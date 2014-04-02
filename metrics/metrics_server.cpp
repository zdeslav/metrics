#include "stdafx.h"
#include "metrics_server.h"
#include "windows.h"
#include <chrono>

namespace metrics
{
    storage g_storage;

    server_config::server_config(unsigned int port) :
        m_port(port),
        m_callback([]{}), // NOP callback
        m_flush_period(60)
    {
        ensure_winsock_started();
    }

    server_config& server_config::flush_every(unsigned int period) {
        m_flush_period = period;
        return *this;
    }

    server_config& server_config::pre_flush(FLUSH_CALLBACK callback) {
        if (!callback)
        {
            char msg[256];
            _snprintf_s(msg, _countof(msg), _TRUNCATE, "Pre-flush callback must not be NULL");
            dbg_print(msg);
            throw config_exception(msg);
        }
        m_callback = callback;
        return *this;
    }

    server_config& server_config::log(const char* target) {
        // todo
        return *this;
    }

    timer_data process_timer(const std::string& name, std::vector<int> values)
    {
        timer_data data = { name, values.size(), 0, 0, 0, 0, 0 };   
        if (data.count == 0) return data;

        data.min = values[0];
        data.max = data.min;
        long long square_sum = 0;  // needed for stddev
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

    stats flush_metrics(const storage& storage, unsigned int period_ms)
    {
        stats stats;
        stats.timestamp = timer::now();
        dbg_print("processing metrics: C[%d], G[%d], H[%d]",
                  storage.counters.size(), 
                  storage.gauges.size(), 
                  storage.timers.size());

        auto period =  period_ms / 1000.0;

        for (auto c : storage.counters)
        {
            stats.counters[c.first] = c.second / period;
        }
        for (auto g : storage.gauges)
        {
            stats.gauges[g.first] = g.second;
        }
        for (auto t : storage.timers)
        {
            stats.timers[t.first] = process_timer(t.first, t.second);
        }

        return stats; // todo: move
    }

    void process_metric(char* buff, size_t len)
    {
        auto pipe_pos = strrchr(buff, '|');
        auto colon_pos = strrchr(buff, ':');
        if (!colon_pos || !pipe_pos) {
            dbg_print("unknown metric: %s", buff);
            return;
        }

        metric_type metric;
        if (strcmp(pipe_pos, "|h") == 0) metric = histogram;
        else if (strcmp(pipe_pos, "|g") == 0) metric = gauge;
        else if (strcmp(pipe_pos, "|c") == 0) metric = counter;
        else if (strcmp(pipe_pos, "|ms") == 0) metric = histogram;
        else {
            dbg_print("unknown metric type: %s", pipe_pos);
            return;
        }

        *pipe_pos = '\0';
        *colon_pos = '\0';
        colon_pos++;
        std::string metric_name = buff;
        int value = atol(colon_pos);

        dbg_print("storing metric %d: %s [%d]", metric, metric_name.c_str(), value);

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
        using namespace std::chrono;

        const int BUFSIZE = 4096;

        server_config* cfg = static_cast<server_config*>(params);
        auto port = cfg->port();
        auto period_ms = cfg->flush_period() * 1000;
        auto flush_cb = cfg->callback();
        auto backends = cfg->backends();
        delete cfg;

        struct sockaddr_in myaddr;      // our address 
        struct sockaddr_in remaddr;     // remote address 
        int addrlen = sizeof(remaddr);  // length of addresses 
        int recvlen;                    // # bytes received 
        int fd;                         // our socket 
        char buf[BUFSIZE];              // receive buffer 

        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // create a UDP socket
            dbg_print("cannot create server socket: error: %d", WSAGetLastError());
            return 1;
        }

        auto start = timer::now();
        /* bind the socket to any valid IP address and a specific port */
        memset((char *)&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
            dbg_print("bind failed");
            return 1;
        }

        dbg_print("inproc server listening at port %d", port);
        
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
                if (recvlen > 0 && recvlen < BUFSIZE) {
                    buf[recvlen] = 0;
                    if (strcmp(buf, "stop") == 0) {
                        dbg_print(" > received STOP cmd, stopping server");
                        return 0;
                    }
                    dbg_print(" > received:%s (%d bytes)", buf, recvlen);
                    process_metric(buf, recvlen);
                }                 
            }

            auto ellapsed = timer::now() - start;
            milliseconds ms = duration_cast<milliseconds>(ellapsed);

            if (ms.count() >= period_ms)
            {
                start = timer::now();
                flush_cb();
                stats stats = flush_metrics(g_storage, period_ms);
                g_storage.clear();
                for (auto be : backends) be(stats);
                auto time_ms = duration_cast<milliseconds>(timer::now() - start);
                dbg_print("flush took %d ms", (int)time_ms.count());
            }
        }
    }

    server server::run(const server_config& cfg)
    {
        dbg_print("starting inproc server....");
        DWORD thread_id;
        CreateThread(NULL, 0, ThreadProc, new server_config(cfg), 0, &thread_id);
        return server(cfg);
    }

    void server::stop()
    {
        dbg_print("sending stop cmd...");
        const char* cmd = "stop";
        send_to_server(cmd, strlen(cmd));
    }
}