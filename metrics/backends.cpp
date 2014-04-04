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
        FOR_EACH (auto& c, stats.counters)
        {
            printf(" C: %s - %.2f 1/s\n", c.first.c_str(), c.second);
        }
        FOR_EACH (auto& g, stats.gauges)
        {
            printf(" G: %s - %lld\n", g.first.c_str(), g.second);
        }
        FOR_EACH (auto& t, stats.timers)
        {
            printf(" H: %s\n", t.second.dump().c_str());
        }
    }

    void file_backend::operator()(const stats& stats)
    {
        std::ofstream ofs;
        ofs.open(m_filename, std::ofstream::out | std::ofstream::app);

        ofs << "@ TS: " << timer::to_string(stats.timestamp) << "\n";

        FOR_EACH (auto& c, stats.counters)
        {
            ofs << " C: " << c.first.c_str() << " - " << c.second << "1/s\n";
        }
        FOR_EACH (auto& g, stats.gauges)
        {
            ofs << " G: " << g.first.c_str() << " - " << g.second << "\n";
        }
        FOR_EACH (auto& t, stats.timers)
        {
            ofs << " H: " << t.second.dump() << "\n";
        }
        ofs << "----------------------------------------------\n";

        ofs.close();
    }
}