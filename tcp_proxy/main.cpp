#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <chrono>

#include "tcp_proxy.hpp"

static void system_cmd (const char* label, const char* cmd_str)
{
    printf ("%s ---- %s\n\n", label, cmd_str);
    fflush (NULL);
    system (cmd_str);
}

static void dump_stats (const char* stats_file, app_stats* stats) 
{
    json j;
    
    stats->dump_json(j);

    std::ofstream stats_stream(stats_file);
    stats_stream << j << std::endl;
}

int main(int /*argc*/, char **argv) 
{
    char* mode = argv[1];
    char* cfg_name = argv[2];

    return 0;
}

