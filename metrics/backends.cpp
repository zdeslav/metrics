#include "stdafx.h"
#include "backends.h"
#include "metrics_server.h"
#include <fstream> 
#include <ctime>
#include <string>

namespace metrics
{
    void console_backend::operator()(const stats& stats)
    {
        for (auto& c : stats.counters)
        {
            printf(" C: %s - %.2f 1/s\n", c.first.c_str(), c.second);
        }
        for (auto& g : stats.gauges)
        {
            printf(" G: %s - %lld\n", g.first.c_str(), g.second);
        }
        for (auto& t : stats.timers)
        {
            printf(" H: %s\n", t.second.dump().c_str());
        }
    }

    void file_backend::operator()(const stats& stats)
    {
        std::ofstream ofs;
        ofs.open(m_filename, std::ofstream::out | std::ofstream::app);

        char buff[64];
        auto ts = timer::to_time_t(stats.timestamp);
        ctime_s(buff, _countof(buff), &ts);

        ofs << "@ " << buff << "\n";

        for (auto& c : stats.counters)
        {
            ofs << " C: " << c.first.c_str() << " - " << c.second << "1/s\n";
        }
        for (auto& g : stats.gauges)
        {
            ofs << " G: " << g.first.c_str() << " - " << g.second << "\n";
        }
        for (auto& t : stats.timers)
        {
            ofs << " H: " << t.second.dump() << "\n";
        }
        ofs << "----------------------------------------------\n";

        ofs.close();
    }
}