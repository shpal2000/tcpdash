#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "./apps/tls_server/tls_server.hpp"
#include "./apps/tls_client/tls_client.hpp"

#define MAX_CONFIG_DIR_PATH 256
#define MAX_CONFIG_FILE_PATH 512
#define MAX_EXE_FILE_PATH 512
#define MAX_VOLUME_STRING 512
#define MAX_CMD_LEN 2048
#define MAX_CMD_LEN 2048
#define MAX_RESULT_DIR_PATH 2048
#define MAX_FILE_PATH_LEN 2048
#define RUN_DIR_PATH "/rundir/"
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
char registry_dir [MAX_REGISTRY_DIR_PATH];
char zone_cname [MAX_ZONE_CNAME];

ev_sockstats* zone_ev_sockstats = nullptr;
tls_server_stats* zone_tls_server_stats = nullptr;
tls_client_stats* zone_tls_client_stats = nullptr;

static void system_cmd (const char* label, const char* cmd_str)
{
    printf ("%s ---- %s\n\n", label, cmd_str);
    fflush (NULL);
    system (cmd_str);
}

static void dump_stats (const char* stats_file, ev_sockstats* stats) 
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
                                    , char* cfg_name
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
        auto zone = z_it.value ();
        auto a_list = zone["app_list"];

        for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it)
        {
            auto app = a_it.value ();
            auto app_label = app["app_label"].get<std::string>();
            
            sprintf (curr_dir_file, "%s%s", registry_dir, app_label.c_str());
            std::ofstream o_progress_file(curr_dir_file);
            o_progress_file << "0";
        }
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

static void create_result_entry (json cfg_json
                                    , char* cfg_name
                                    , char* run_tag)
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
        auto zone_label = zone ["zone_label"].get<std::string>();
        
        sprintf (curr_dir_file, "%s%s", result_dir, zone_label.c_str());
        sprintf (cmd_str, "mkdir %s", curr_dir_file);
        system_cmd ("create zone dir", cmd_str);

        auto a_list = zone["app_list"];

        for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it)
        {
            auto app = a_it.value ();
            auto app_label = app["app_label"].get<std::string>();
            
            sprintf (curr_dir_file, "%s%s/%s"
                        , result_dir, zone_label.c_str(), app_label.c_str());

            sprintf (cmd_str, "mkdir %s", curr_dir_file);
            system_cmd ("create app dir", cmd_str);
        }
    }
}

static void start_zones (json cfg_json
                            , char* cfg_name
                            , char* run_tag
                            , char* host_run_dir)
{
    sprintf (tlspack_exe_file, "%sbin/tlspack.exe", RUN_DIR_PATH);

    sprintf (ssh_host_file, "%s.ssh/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    sprintf (volume_string, "--volume=%s:%s", host_run_dir, RUN_DIR_PATH);

    //launch containers
    auto z_list = cfg_json["zones"];
    int z_index = -1;
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        z_index++;
        sprintf (zone_cname, "%s_%d", cfg_name, z_index+1);

        sprintf (cmd_str,
                "ssh -i %s.ssh/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=bridge --privileged "
                "--name %s -it -d %s tgen %s zone %s %s %d %s config_zone",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                zone_cname, volume_string, tlspack_exe_file, cfg_name, run_tag, z_index, zone_cname);
                
        system_cmd ("zone start", cmd_str);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static void stop_zones (json cfg_json
                            , char* cfg_name)
{
    sprintf (ssh_host_file, "%s.ssh/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    auto z_list = cfg_json["zones"];
    int z_index = -1;
    for (auto z_it = z_list.begin(); z_it != z_list.end(); ++z_it)
    {
        z_index++;
        sprintf (zone_cname, "%s_%d", cfg_name, z_index+1);

        sprintf (cmd_str,
                "ssh -i %s.ssh/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker rm -f %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                zone_cname);
                
        system_cmd ("zone stop", cmd_str);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    sprintf (zone_cname, "%s_0", cfg_name);
    sprintf (cmd_str,
                "ssh -i %s.ssh/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker rm -f %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                zone_cname);
    system_cmd ("zone0 stop", cmd_str);

    remove_registry_entry ();
}

static void config_zone (json cfg_json
                            , char* cfg_name
                            , int z_index)
{
    sprintf (zone_cname, "%s_%d", cfg_name, z_index+1);

    sprintf (ssh_host_file, "%s.ssh/host", RUN_DIR_PATH);
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
                "ssh -i %s.ssh/id_rsa -tt "
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

static std::vector<ev_app*>* create_app_list (json cfg_json, int z_index)
{
    std::vector<ev_app*> *app_list = nullptr;

    auto a_list = cfg_json["zones"][z_index]["app_list"];
    int a_index = -1;
    for (auto a_it = a_list.begin(); a_it != a_list.end(); ++a_it)
    {
        if (zone_ev_sockstats == nullptr)
        {
            zone_ev_sockstats = new ev_sockstats();
        }

        a_index++;
        auto app_json = a_it.value ();
        const char* app_type = app_json["app_type"].get<std::string>().c_str();

        ev_app* next_app = nullptr;
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
            if (app_list == nullptr)
            {
                app_list = new std::vector<ev_app*>;
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

int main(int argc, char **argv) 
{
    char* mode = argv[1];
    char* cfg_name = argv[2];

    sprintf (registry_dir, "%sregistry/%s/", RUN_DIR_PATH, cfg_name);

    sprintf (cfg_dir, "%straffic/%s/", RUN_DIR_PATH, cfg_name);
    sprintf (cfg_file, "%s%s", cfg_dir, "config.json");
    std::ifstream cfg_stream(cfg_file);
    json cfg_json = json::parse(cfg_stream);

    if (strcmp(mode, "start") == 0)
    {
        char* run_tag = argv[3];
        char* host_run_dir = argv[4];
        
        sprintf (result_dir, "%sresults/%s/%s/", RUN_DIR_PATH, cfg_name, run_tag);

        if ( is_registry_entry_exit() )
        {
            printf ("%s running : stop before starting again\n", cfg_name);
            exit (-1);
        }

        create_registry_entry (cfg_json, cfg_name, run_tag);

        create_result_entry (cfg_json, cfg_name, run_tag);

        start_zones (cfg_json, cfg_name, run_tag, host_run_dir);

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
        char* zone_cname = argv[5];
        
        sprintf (result_dir, "%sresults/%s/%s/", RUN_DIR_PATH, cfg_name, run_tag);

        const char* config_zone_flag = "skip_config_zone";
        if (argc > 6)
        {
            config_zone_flag = argv[6];
        }
        
        if ( strcmp(config_zone_flag, "config_zone") == 0 )
        {
            config_zone (cfg_json, cfg_name, z_index);
        }

        signal(SIGPIPE, SIG_IGN);

        std::vector<ev_app*> *app_list = create_app_list (cfg_json, z_index);

        if ( app_list )
        {
            uint64_t mu_ticks = 0;

            while (1)
            {
                for (ev_app* app_ptr : *app_list)
                {
                    app_ptr->run_iter ();
                }

                std::this_thread::sleep_for(std::chrono::microseconds(1));

                // mu_ticks++;
                // if (mu_ticks == 0)
                // {
                //     mu_ticks = 0;

                //     sprintf (curr_dir_file,
                //             "%s%s/container_%d/"
                //             "ev_sockstats_container.json",
                //             result_dir, mode, c_index);

                //     dump_stats (curr_dir_file, ev_sockstats_container);

                //     if (tls_server_stats_container)
                //     {
                        
                //         sprintf (curr_dir_file,
                //                 "%s%s/container_%d/"
                //                 "tls_server_stats_container.json",
                //                 result_dir, mode, c_index);

                //         dump_stats (curr_dir_file, tls_server_stats_container);
                //     }

                //     if (tls_client_stats_container)
                //     {
                        
                //         sprintf (curr_dir_file,
                //                 "%s%s/container_%d/"
                //                 "tls_client_stats_container.json",
                //                 result_dir, mode, c_index);

                //         dump_stats (curr_dir_file, tls_client_stats_container);
                //     }

                //     a_index = -1;
                //     for (ev_app* app_ptr : app_list)
                //     {
                //         a_index++;
                //         sprintf (curr_dir_file,
                //                 "%s%s/container_%d/app_%d/"
                //                 "%s_stats_app.json",
                //                 result_dir, mode, c_index, a_index,
                //                 app_ptr->get_app_type() );

                //         dump_stats ( curr_dir_file, app_ptr->get_app_stats() );
                //     }
                // }
            }
        }
    }

    return 0;
}
