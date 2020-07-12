#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <chrono>

#include "./apps/tls_server/tls_server.hpp"
#include "./apps/tls_client/tls_client.hpp"

#define MAX_CONFIG_DIR_PATH 256
#define MAX_CONFIG_FILE_PATH 512
#define MAX_EXE_FILE_PATH 512
#define MAX_VOLUME_STRING 1024
#define MAX_CMD_LEN 2048
#define MAX_CMD_LEN 2048
#define MAX_RESULT_DIR_PATH 2048
#define MAX_FILE_PATH_LEN 2048
#define RUN_DIR_PATH "/rundir/"
#define SRC_DIR_PATH "/tcpdash/"
#define MAX_REGISTRY_DIR_PATH 2048
#define MAX_ZONE_CNAME 64
char cmd_str [MAX_CMD_LEN];
char cfg_dir [MAX_CONFIG_DIR_PATH];
char cfg_file [MAX_CONFIG_FILE_PATH];
char ssh_host_file [MAX_CONFIG_FILE_PATH];
char tlspack_exe_file [MAX_EXE_FILE_PATH];
char volume_string [MAX_VOLUME_STRING];
char result_dir [MAX_RESULT_DIR_PATH];
char curr_dir_file [MAX_FILE_PATH_LEN];
char zone_file [MAX_FILE_PATH_LEN];
char registry_dir [MAX_REGISTRY_DIR_PATH];
char zone_cname [MAX_ZONE_CNAME];

app_stats* zone_ev_sockstats = nullptr;
tls_server_stats* zone_tls_server_stats = nullptr;
tls_client_stats* zone_tls_client_stats = nullptr;

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

static bool is_registry_entry_exit (/*json cfg_json
                                    , char* cfg_name*/)
{
    sprintf (curr_dir_file, "%stag.txt", registry_dir);
    std::ifstream i_tagfile(curr_dir_file);
    return i_tagfile.good ();
}

static void create_registry_entry (json cfg_json
                                    , char* run_tag)
{
    //create registry dir
    sprintf (cmd_str, "mkdir %s", registry_dir); 
    system_cmd ("create registry dir", cmd_str);

    sprintf (curr_dir_file, "%stag.txt", registry_dir);
    std::ofstream o_tagfile(curr_dir_file);
    o_tagfile << run_tag;

    auto z_list = cfg_json["zones"];
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        int zone_enabe = z_it.value()["enable"].get<int>();
        if (zone_enabe == 0) {
            continue;
        }

        bool is_app_enable = false;
        auto a_list = z_it.value()["app_list"];
        for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it) {
            auto app_json = a_it.value ();
            int app_enable = app_json["enable"].get<int>();
            if (app_enable) {
                is_app_enable = true;
                break;
            }
        }

        if (not is_app_enable) {
            continue;
        }

        auto zone_label = z_it.value()["zone_label"].get<std::string>();
        sprintf (curr_dir_file, "%s%s", registry_dir, zone_label.c_str());
        std::ofstream f(curr_dir_file);
        f << "0";
    }

    sprintf (curr_dir_file, "%smaster", registry_dir);
    std::ofstream o_progress_master(curr_dir_file);
    o_progress_master << "0";
}

static void remove_registry_entry (/*json cfg_json
                                    , char* cfg_name*/)
{
    //remove registry dir
    sprintf (cmd_str, "rm -rf %s", registry_dir); 
    system_cmd ("delete registry dir", cmd_str);
}

static void create_result_entry (json cfg_json)
{
    //remove result dir
    sprintf (cmd_str, "rm -rf %s", result_dir); 
    system_cmd ("remove result dir if exist", cmd_str);

    //create result dir
    sprintf (cmd_str, "mkdir -p %s", result_dir); 
    system_cmd ("create result dir", cmd_str);

    //create zone dirs
    auto z_list = cfg_json["zones"];
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        auto zone = z_it.value ();
        int zone_enabe = zone["enable"].get<int>();
        if (zone_enabe == 0) {
            continue;
        }

        auto zone_label = zone ["zone_label"].get<std::string>();
        
        sprintf (curr_dir_file, "%s%s", result_dir, zone_label.c_str());
        sprintf (cmd_str, "mkdir %s", curr_dir_file);
        system_cmd ("create zone dir", cmd_str);

        auto a_list = zone["app_list"];
        for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it)
        {
            auto app = a_it.value ();
            auto app_label = app["app_label"].get<std::string>();
            int app_enable = app["enable"].get<int>();

            if (app_enable ==0){
                continue;
            }

            sprintf (curr_dir_file, "%s%s/%s"
                        , result_dir, zone_label.c_str(), app_label.c_str());
            sprintf (cmd_str, "mkdir %s", curr_dir_file);
            system_cmd ("create app dir", cmd_str);

            auto srv_list = app["srv_list"];
            for (auto s_it = srv_list.begin(); s_it != srv_list.end(); ++s_it)
            {
                auto srv = s_it.value ();
                auto srv_label = srv["srv_label"].get<std::string>();
                int srv_enable = srv["enable"].get<int>();

                if (srv_enable == 0) {
                    continue;
                }

                sprintf (curr_dir_file,
                        "%s%s/"
                        "%s/%s",
                        result_dir, zone_label.c_str(), 
                        app_label.c_str(), srv_label.c_str());
                sprintf (cmd_str, "mkdir %s", curr_dir_file);
                system_cmd ("create srv dir", cmd_str);
            }

            auto cs_grp_list = app["cs_grp_list"];
            for (auto cs_it = cs_grp_list.begin(); 
                    cs_it != cs_grp_list.end(); ++cs_it)
            {
                auto cs_grp = cs_it.value ();
                auto cs_grp_label = cs_grp["cs_grp_label"].get<std::string>();
                int cs_grp_enable = cs_grp["enable"].get<int>();

                if (cs_grp_enable == 0) {
                    continue;
                }

                sprintf (curr_dir_file,
                        "%s%s/"
                        "%s/%s",
                        result_dir, zone_label.c_str(), 
                        app_label.c_str(), cs_grp_label.c_str());
                sprintf (cmd_str, "mkdir %s", curr_dir_file);
                system_cmd ("create cs_grp dir", cmd_str);
            }
        }
    }
}

static void start_zones (json cfg_json
                            , char* cfg_name
                            , char* run_tag
                            , char* host_run_dir
                            , int is_debug
                            , char* host_src_dir)
{
    sprintf (ssh_host_file, "%ssys/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    //launch containers
    auto z_list = cfg_json["zones"];
    int z_index = -1;
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        z_index++;
        int zone_enabe = z_it.value()["enable"].get<int>();
        if (zone_enabe == 0) {
            continue;
        }

        auto zone_label = z_it.value()["zone_label"].get<std::string>();
        sprintf (zone_cname, "%s-%s", cfg_name, zone_label.c_str());

        if (is_debug == 1)
        {
            sprintf (volume_string,
                        "--volume=%s:%s "
                        "--volume=%s:%s", 
                        host_run_dir, RUN_DIR_PATH,
                        host_src_dir, SRC_DIR_PATH);

            sprintf (cmd_str,
                    "ssh -i %ssys/id_rsa -tt "
                    // "-o LogLevel=quiet "
                    "-o StrictHostKeyChecking=no "
                    "-o UserKnownHostsFile=/dev/null "
                    "%s@%s "
                    "sudo docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=bridge --privileged "
                    "--name %s -it -d %s tgen /bin/bash",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    zone_cname, volume_string);
        }
        else
        {
            sprintf (tlspack_exe_file, "%sbin/tlspack.exe", RUN_DIR_PATH);

            if (is_debug == 2) {
                sprintf (volume_string,
                            "--volume=%s:%s "
                            "--volume=%s:%s", 
                            host_run_dir, RUN_DIR_PATH,
                            host_src_dir, SRC_DIR_PATH);
            } else {
                sprintf (volume_string
                            , "--volume=%s:%s"
                            , host_run_dir, RUN_DIR_PATH);
            }

            sprintf (cmd_str,
                    "ssh -i %ssys/id_rsa -tt "
                    // "-o LogLevel=quiet "
                    "-o StrictHostKeyChecking=no "
                    "-o UserKnownHostsFile=/dev/null "
                    "%s@%s "
                    "sudo docker run --env LD_LIBRARY_PATH='/root/openssl-1.1.1d/local/lib/' --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=bridge --privileged "
                    "--name %s -it -d %s tgen %s zone %s "
                    "%s %d config_zone %d",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    zone_cname, volume_string, tlspack_exe_file, cfg_name,
                    run_tag, z_index, is_debug);
        }
        
        system_cmd ("zone start", cmd_str);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static void stop_zones (json cfg_json
                            , char* cfg_name)
{
    sprintf (ssh_host_file, "%ssys/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    auto z_list = cfg_json["zones"];
    int z_index = -1;
    std::string c_list = "";
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        z_index++;
        int zone_enabe = z_it.value()["enable"].get<int>();
        if (zone_enabe == 0) {
            continue;
        }

        auto zone_label = z_it.value()["zone_label"].get<std::string>();
        sprintf (zone_cname, "%s-%s", cfg_name, zone_label.c_str());

        c_list.append (" ");
        c_list.append (zone_cname);
    }

    sprintf (zone_cname, "%s-root", cfg_name);
    c_list.append (" ");
    c_list.append (zone_cname);

    sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker rm -f %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                c_list.c_str ());
    system_cmd ("zone stop", cmd_str);

    remove_registry_entry ();
}

static void config_zone (json cfg_json
                            , char* cfg_name
                            , int z_index)
{
    auto zone_label 
        = cfg_json["zones"][z_index]["zone_label"].get<std::string>();
    sprintf (zone_cname, "%s-%s", cfg_name, zone_label.c_str());

    sprintf (ssh_host_file, "%ssys/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    auto topology = cfg_json["zones"][z_index]["zone_type"].get<std::string>();
    if ( strcmp(topology.c_str(), "single-interface") == 0 )
    {
        auto macvlan = cfg_json["zones"][z_index]["macvlan"].get<std::string>();
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker network connect %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                macvlan.c_str(), zone_cname);
        system_cmd ("connect docker network", cmd_str);

        auto iface = cfg_json["zones"][z_index]["iface"].get<std::string>();
        sprintf (cmd_str,
                "ip link set dev %s up",
                iface.c_str());
        system_cmd ("dev up", cmd_str);

        sprintf (cmd_str,
                "ip route add default dev %s table 200",
                iface.c_str());
        system_cmd ("dev default gw", cmd_str);

        // sprintf (cmd_str,
        //         "ifconfig %s txqueuelen 300000",
        //         iface.c_str());
        // system_cmd ("dev txqueuelen", cmd_str);

        auto subnets = cfg_json["zones"][z_index]["subnets"];
        for (auto it = subnets.begin(); it != subnets.end(); ++it)
        {
            auto subnet = it.value().get<std::string>();
            sprintf (cmd_str,
                    "ip -4 route add local %s dev lo",
                    subnet.c_str());
            system_cmd ("ip configure", cmd_str);

            sprintf (cmd_str,
                    "ip rule add from %s table 200",
                    subnet.c_str());
            system_cmd ("ip source routing", cmd_str);
        }
    }
    else
    {   
        printf ("unknown zone type %s\n", topology.c_str());
        exit (-1);
    }
}

static std::vector<app*>* create_app_list (json cfg_json, int z_index)
{
    std::vector<app*> *app_list = nullptr;

    auto a_list = cfg_json["zones"][z_index]["app_list"];
    int a_index = -1;
    for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it)
    {
        if (zone_ev_sockstats == nullptr)
        {
            zone_ev_sockstats = new app_stats();
        }

        a_index++;
        auto app_json = a_it.value ();
        const char* app_type = app_json["app_type"].get<std::string>().c_str();
        const char* app_label = app_json["app_label"].get<std::string>().c_str();
        int app_enable = app_json["enable"].get<int>();

        if (app_enable ==0){
            continue;
        }

        app* next_app = nullptr;
        if ( strcmp("tls_server", app_type) == 0 )
        {
            if (zone_tls_server_stats == nullptr)
            {
                zone_tls_server_stats = new tls_server_stats ();
            }

            next_app = new tls_server_app (app_json
                                            , zone_tls_server_stats
                                            , zone_ev_sockstats);

        }
        else if ( strcmp("tls_client", app_type) == 0 )
        {
            if (zone_tls_client_stats == nullptr)
            {
                zone_tls_client_stats = new tls_client_stats ();
            }
            
            next_app = new tls_client_app (app_json
                                            , zone_tls_client_stats
                                            , zone_ev_sockstats);
        }

        if (next_app)
        {
            next_app->set_app_type (app_type);
            next_app->set_app_label (app_label);

            if (app_list == nullptr)
            {
                app_list = new std::vector<app*>;
            }

            app_list->push_back (next_app);
        }
        else
        {
            printf ("unknown app_type %s\n", app_type);
            exit (-1);
        }
    }

    return app_list;
}

static bool all_zones_initialized (json cfg_json)
{
    bool ret = true;
    auto z_list = cfg_json["zones"];
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        int zone_enabe = z_it.value()["enable"].get<int>();
        if (zone_enabe == 0) {
            continue;
        }
        
        auto zone_label = z_it.value()["zone_label"].get<std::string>();
        sprintf (curr_dir_file, "%s%s", registry_dir, zone_label.c_str());
        std::ifstream f(curr_dir_file);
        std::ostringstream ss;
        std::string str;
        ss << f.rdbuf();
        str = ss.str();
        if (atoi(str.c_str()) < 1)
        {
            ret = false;
            break;
        }
    }
    return ret;
}

static void update_registry_state (const char* registry_file, int state)
{
    char state_str [128];
    sprintf (state_str, "%d", state);
    std::ofstream f(registry_file);
    f << state_str; 
}

int main(int /*argc*/, char **argv) 
{
    char* mode = argv[1];
    char* cfg_name = argv[2];

    sprintf (registry_dir, "%sregistry/%s/", RUN_DIR_PATH, cfg_name);

    sprintf (cfg_dir, "%straffic/%s/", RUN_DIR_PATH, cfg_name);
    sprintf (cfg_file, "%s%s", cfg_dir, "config.json");
    std::ifstream cfg_stream(cfg_file);
    // printf ("cfg_file : %s\n", cfg_file);
    json cfg_json = json::parse(cfg_stream);

    if (strcmp(mode, "start") == 0)
    {
        char* run_tag = argv[3];
        char* host_run_dir = argv[4];
        int is_debug = atoi (argv[5]);
        char* host_src_dir = argv[6];
        
        sprintf (result_dir, "%straffic/%s/%s/%s", RUN_DIR_PATH, cfg_name
                                                    , "results", run_tag);

        if ( is_registry_entry_exit() )
        {
            printf ("%s running : stop before starting again\n", cfg_name);
            exit (-1);
        }

        create_registry_entry (cfg_json, run_tag);

        create_result_entry (cfg_json);

        start_zones (cfg_json, cfg_name, run_tag, host_run_dir
                                        , is_debug, host_src_dir);

        while (1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else if (strcmp(mode, "stop") == 0)
    {
        if ( not is_registry_entry_exit() )
        {
            printf ("%s not running\n", cfg_name);
            exit (-1);
        }

        stop_zones (cfg_json, cfg_name);
    }
    else //zone
    {   
        char* run_tag = argv[3];
        int z_index = atoi (argv[4]);
        char* config_zone_flag = argv[5];
        
        auto zone_label 
            = cfg_json["zones"][z_index]["zone_label"].get<std::string>();
        sprintf (zone_file, "%s%s", registry_dir, zone_label.c_str());

        sprintf (zone_cname, "%s-%s", cfg_name, zone_label.c_str());

        sprintf (result_dir, "%straffic/%s/%s/%s", RUN_DIR_PATH, cfg_name
                                                    , "results", run_tag);
        
        if ( strcmp(config_zone_flag, "config_zone") == 0 )
        {
            config_zone (cfg_json, cfg_name, z_index);
        }

        signal(SIGPIPE, SIG_IGN);

        OpenSSL_add_ssl_algorithms ();
        SSL_load_error_strings ();

        auto iface = cfg_json["zones"][z_index]["iface"].get<std::string>();
        auto tcpdump_s = cfg_json["zones"][z_index]["tcpdump"].get<std::string>();
        sprintf (cmd_str,
            "tcpdump -i %s -n %s -w /rundir/bin/%s.pcap&",
                iface.c_str(), tcpdump_s.c_str(), zone_cname);
        system_cmd ("tcpdump start", cmd_str);

        std::this_thread::sleep_for(std::chrono::seconds(8));

        std::vector<app*> *app_list = create_app_list (cfg_json, z_index);
        update_registry_state (zone_file, 1);

        if ( app_list )
        {

            while (not all_zones_initialized (cfg_json))
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            update_registry_state (zone_file, 2);

            std::chrono::time_point<std::chrono::system_clock> start, end;
            start = std::chrono::system_clock::now();
            int tick_5sec = 0;
            bool is_tick_sec = false;
            while (1)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                end = std::chrono::system_clock::now();
                auto ms_elapsed 
                    = std::chrono::duration_cast<std::chrono::milliseconds>
                    (end-start);

                if (ms_elapsed.count() >= 1000)
                {
                    start = end;
                    is_tick_sec = true;
                    tick_5sec++;
                }

                for (app* app_ptr : *app_list)
                {
                    app_ptr->run_iter (is_tick_sec);
                }

                if (is_tick_sec) 
                {
                    zone_ev_sockstats->tick_sec ();

                    is_tick_sec = false;
                }

                if (tick_5sec == 5)
                {
                    tick_5sec = 0;

                    sprintf (curr_dir_file,
                            "%s%s/"
                            "ev_sockstats.json",
                            result_dir, zone_label.c_str());
                    dump_stats (curr_dir_file, zone_ev_sockstats);

                    if (zone_tls_server_stats)
                    {
                        sprintf (curr_dir_file,
                                "%s%s/"
                                "tls_server_stats.json",
                                result_dir, zone_label.c_str());
                        dump_stats (curr_dir_file, zone_tls_server_stats);
                    }

                    if (zone_tls_client_stats)
                    {
                        sprintf (curr_dir_file,
                                "%s%s/"
                                "tls_client_stats.json",
                                result_dir, zone_label.c_str());
                        dump_stats (curr_dir_file, zone_tls_client_stats);
                    }

                    for (app* app_ptr : *app_list)
                    {
                        sprintf (curr_dir_file,
                            "%s%s/"
                            "%s/%s_stats.json",
                            result_dir, zone_label.c_str(),
                            app_ptr->get_app_label(), app_ptr->get_app_type());
                        dump_stats (curr_dir_file
                                    , (app_stats*)app_ptr->get_app_stats());

                        ev_stats_map* app_stats_map 
                            = app_ptr->get_app_stats_map ();
                        for (auto it=app_stats_map->begin(); 
                                it!=app_stats_map->end(); ++it)
                        {
                            sprintf (curr_dir_file,
                            "%s%s/"
                            "%s/%s/"
                            "%s_stats.json",
                            result_dir, zone_label.c_str(),
                            app_ptr->get_app_label(), it->first.c_str(),
                            app_ptr->get_app_type());
                            dump_stats (curr_dir_file, (app_stats*)it->second);  
                        }
                    }
                }
            }

            for (std::string line; std::getline(std::cin, line);) 
            {
                std::cout << line << std::endl;
            }
        }
        else
        {
            printf ("no apps!\n");
            exit (-1);
        }
    }
    return 0;
}


//docker rm -f  try2_0 try2_1 try2_2 try2_3 try2_4 try2_5 try2_6 try2_7 try2_8 try2_9 try2_10 try2_11 try2_12