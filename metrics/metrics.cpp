#include "stdafx.h"
#include "metrics.h"
#include "string.h"
#include "stdlib.h"
#include <cstdarg>

using timer = std::chrono::high_resolution_clock;

#define thread_local __declspec( thread )

namespace metrics
{
    client_config g_client;

    void ensure_winsock_started()
    {
        SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        bool started = s != INVALID_SOCKET || WSAGetLastError() != WSANOTINITIALISED;
        if (s != INVALID_SOCKET) closesocket(s);
        if (started) return;

        WORD wVersionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;

        int err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0)
        {
            char msg[256];
            _snprintf_s(msg, _countof(msg), _TRUNCATE, "WSAStartup failed with error: %d", err);
            dbg_print(msg);
            throw config_exception(msg);
        }
    }

    client_config& setup_client(const char* server, unsigned int port)
    {
        ensure_winsock_started();

        g_client.m_server = server;
        g_client.m_port = port;

        struct hostent *hp;     /* host information */
        /* fill in the server's address and data */
        memset((char*)&g_client.m_svr_address, 0, sizeof(g_client.m_svr_address));
        g_client.m_svr_address.sin_family = AF_INET;
        g_client.m_svr_address.sin_port = htons(g_client.m_port);

        /* look up the address of the server given its name */
        hp = gethostbyname(g_client.m_server.c_str());
        if (!hp) {
            char msg[256];
            _snprintf_s(msg, 
                        _countof(msg), 
                        _TRUNCATE, 
                        "Could not obtain address of %s. Error: %d", 
                        g_client.m_server.c_str(),
                        WSAGetLastError());
            dbg_print(msg);
            throw config_exception(msg);
        }

        /* put the host's address into the server address structure */
        memcpy((void *)&g_client.m_svr_address.sin_addr, hp->h_addr_list[0], hp->h_length);

        return g_client;
    }

    client_config::client_config() :
        m_debug(false),
        m_defaults_period(0), // 0 - no tracking
        m_port(0),
        m_namespace("stats")
    {;}

    client_config& client_config::set_debug(bool debug) {
        m_debug = debug;
        return *this;
    }
    client_config& client_config::track_default_metrics(unsigned int period) {
        m_defaults_period = period;
        return *this;
    }
    client_config& client_config::set_namespace(const char* ns) {
        m_namespace = ns;
        return *this;
    }

    bool client_config::is_debug() const { return m_debug; }
    const char* client_config::get_namespace() const { return m_namespace.c_str(); }

    template <metric_type m>
    const char* const fmt() {
        static_assert(false, "You must provide a correct specialization");
    }

    template <> const char* const  fmt<histogram>() { return "%s.%s:%d|ms"; }
    template <> const char* const  fmt<gauge>()     { return "%s.%s:%d|g"; }
    template <> const char* const  fmt<counter>()   { return "%s.%s:%d|c"; }

    void send_to_server(const char* txt, size_t len)
    {
        thread_local static SOCKET fd = INVALID_SOCKET;

        if (fd == INVALID_SOCKET) {
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd == INVALID_SOCKET) { // create a UDP socket
                dbg_print("cannot create client socket: error: %d", WSAGetLastError());
                return;
            }
        }

        /* send a message to the server */   
        const sockaddr* paddr = (sockaddr*) g_client.server_address();
        if (sendto(fd, txt, len, 0, paddr, sizeof(*g_client.server_address())) == SOCKET_ERROR) {
            dbg_print("sendto failed, error: %d", WSAGetLastError());
        }
    }

    inline void dbg_print(const char* fmt, ...) {
        if (!g_client.is_debug()) return;

        printf("%d [%d]: ", GetTickCount(), GetCurrentThreadId());
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    template <metric_type m>
    void signal(const char* metric, int val) {
        char txt[256];
        int ret = _snprintf_s(txt,
                              _countof(txt),
                              _TRUNCATE,
                              fmt<m>(),
                              g_client.get_namespace(),
                              metric,
                              val);

        if (ret < 1) {
            dbg_print("error: metric %s didn't fit", metric);
        }
        else {
            send_to_server(txt, strlen(txt));
            dbg_print("%s", txt);
        }
    }

    auto_timer::auto_timer(METRIC_ID metric) : m_metric(metric), m_started_at(timer::now()) {}
    auto_timer::~auto_timer() 
    {
        using namespace std::chrono;
        auto ms = duration_cast<milliseconds>(timer::now() - m_started_at);
        signal<histogram>(m_metric, (int)ms.count()); 
    }

    void increment(METRIC_ID metric, int inc)
    {
        signal<counter>(metric, inc);
    }

    void measure(METRIC_ID metric, int value)
    {
        signal<histogram>(metric, value);
    }

    void set(METRIC_ID metric, int value)
    {
        signal<gauge>(metric, value);
    }
}