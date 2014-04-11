#pragma once

#include "gtest/gtest.h"
#include "../metrics/metrics_server.h"

class fake_server
{
    SOCKET m_sock;

public:
    fake_server(int port = 9999)
    {
        if ((m_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // create a UDP socket
            throw std::runtime_error("cannot create server socket");
        }

        metrics::SOCK_ADDR_IN myaddr(AF_INET, INADDR_LOOPBACK, port);

        if (bind(m_sock, (sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
            closesocket(m_sock);
            throw std::runtime_error("cannot bind server socket");
        }
    }
    ~fake_server()
    {
        closesocket(m_sock);
    }

    std::vector<std::string> get_messages(bool reset_messages = true, int timeout_ms = 50)
    {
        std::vector<std::string> messages;

        sockaddr_in remaddr;
        int addrlen = sizeof(remaddr);
        const int BUFSIZE = 4096;
        char buf[BUFSIZE];
        int maxfd = m_sock;
        fd_set static_rdset, rdset;
        timeval timeout = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };

        FD_ZERO(&static_rdset);
        FD_SET(m_sock, &static_rdset);

        while (true) {
            rdset = static_rdset;
            if (0 == select(maxfd + 1, &rdset, NULL, NULL, &timeout)) break;  // timeout

            if (FD_ISSET(m_sock, &rdset)) {
                int recvlen = recvfrom(m_sock, buf, BUFSIZE, 0, (sockaddr*)&remaddr, &addrlen);
                if (recvlen > 0 && recvlen < BUFSIZE) {
                    buf[recvlen] = 0;
                    messages.push_back(buf);
                }
            }
        }
        return messages;
    }
};

TEST(ClientTest, CheckClientSettings) {
    ASSERT_THROW(metrics::setup_client(""), metrics::config_exception);
    ASSERT_THROW(metrics::setup_client("efgkleioger//*;/&g"), metrics::config_exception);
    ASSERT_NO_THROW(metrics::setup_client("127.0.0.1"));

    auto cfg = metrics::setup_client("127.0.0.1");

    ASSERT_THROW(cfg.track_default_metrics(metrics::all, 0), metrics::config_exception);

    EXPECT_STREQ("stats", cfg.get_namespace());
    cfg.set_namespace("test_ns");
    EXPECT_STREQ("test_ns", cfg.get_namespace());

    cfg.set_debug(true);
    EXPECT_TRUE(cfg.is_debug());
    cfg.set_debug(false);
    EXPECT_FALSE(cfg.is_debug());
}

TEST(ClientTest, CheckClientApi) {
    metrics::setup_client("127.0.0.1");
    fake_server svr;

    metrics::inc("counter");
    metrics::inc("counter", 4);
    metrics::set("gauge", 17);
    metrics::measure("timer", 22);
    metrics::set_delta("gauge", 3);
    metrics::set_delta("gauge", -2);

    auto messages = svr.get_messages();

    EXPECT_EQ("stats.counter:1|c", messages[0]);
    EXPECT_EQ("stats.counter:4|c", messages[1]);
    EXPECT_EQ("stats.gauge:17|g", messages[2]);
    EXPECT_EQ("stats.timer:22|ms", messages[3]);
    EXPECT_EQ("stats.gauge:+3|g", messages[4]);
    EXPECT_EQ("stats.gauge:-2|g", messages[5]);
}

namespace metrics
{
    void process_metric(storage* storage, char* buff, size_t len);
}

TEST(ClientTest, AutoTimer) {
    metrics::setup_client("127.0.0.1");
    fake_server svr;

    {
        metrics::auto_timer _("auto");
        auto ts = metrics::timer::now();
        while (metrics::timer::now() - ts < 50); // wait ~50 ms
    }

    auto messages = svr.get_messages();

    char txt[512];
    strcpy_s(txt, messages[0].c_str());

    metrics::storage store;
    metrics::process_metric(&store, txt, strlen(txt));

    auto name = store.timers.begin()->first;
    auto vec = store.timers.begin()->second;

    EXPECT_EQ("stats.auto", name);
    EXPECT_GE(vec[0], 50);
}

void measured_fn(int timeout_ms)
{
        MEASURE_FN();
        auto ts = metrics::timer::now();
        while (metrics::timer::now() - ts < timeout_ms); 
}

TEST(ClientTest, FunctionTimer) {
    metrics::setup_client("127.0.0.1");
    fake_server svr;

    measured_fn(10);
    measured_fn(20);
    measured_fn(50);

    metrics::storage store;

    FOR_EACH(auto& msg, svr.get_messages())
    {
        char txt[512];
        strcpy_s(txt, msg.c_str());
        metrics::process_metric(&store, txt, strlen(txt));
    }

    auto name = store.timers.begin()->first;
    auto vec = store.timers.begin()->second;

    // gtest generated magic...
    EXPECT_EQ("stats.app.fn.measured_fn", name);
    EXPECT_GE(vec[0], 10);
    EXPECT_GE(vec[1], 20);
    EXPECT_GE(vec[2], 50);
}
