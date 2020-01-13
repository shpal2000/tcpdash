#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "./apps/tls_server/tls_server.hpp"
#include "./apps/tls_client/tls_client.hpp"

#define MAX_CONFIG_DIR_PATH 256
#define MAX_CONFIG_FILE_PATH 512
#define MAX_CMD_LEN 1024
#define RUN_DIR_PATH "/rundir/"
#define TLSPACK_EXE "/tgen_workspace/tcpdash/tlspack/build/tlspack.exe"
#define RUN_VOLUMES "--volume=/home/shirish/tgen_workspace:/tgen_workspace --volume=/home/shirish/rundir:/rundir"
char cmd_str [MAX_CMD_LEN];
char cfg_dir [MAX_CONFIG_DIR_PATH];
char cfg_file [MAX_CONFIG_FILE_PATH];
char ssh_host_file [MAX_CONFIG_FILE_PATH];



int main(int argc, char **argv) 
{
    char* mode = argv[1];
    char* cfg_name = argv[2];
    sprintf (cfg_dir, "%s%s/", RUN_DIR_PATH, cfg_name);
    sprintf (cfg_file, "%s%s", cfg_dir, "config.json");
    std::ifstream cfg_stream(cfg_file);
    json cfg_json = json::parse(cfg_stream);

    sprintf (ssh_host_file, "%s.ssh/host", RUN_DIR_PATH);
    std::ifstream ssh_host_stream(ssh_host_file);
    json ssh_host_json = json::parse(ssh_host_stream);
    auto ssh_host = ssh_host_json["host"].get<std::string>();
    auto ssh_user = ssh_host_json["user"].get<std::string>();
    auto ssh_pass = ssh_host_json["pass"].get<std::string>();

    if (strcmp(mode, "run") == 0)
    {
        //launch server containers
        auto c_list = cfg_json["server"]["containers"];
        int c_index = 0;
        for (auto it = c_list.begin(); it != c_list.end(); ++it)
        {
            sprintf (cmd_str,
                    "ssh -i %s.ssh/id_rsa -tt "
                    "-o LogLevel=quiet "
                    "-o StrictHostKeyChecking=no "
                    "-o UserKnownHostsFile=/dev/null "
                    "%s@%s "
                    "sudo docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=bridge --privileged "
                    "--name tlspack_server_%d --rm -it -d %s tgen %s server %s %d tlspack_server_%d config_topology",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    c_index, RUN_VOLUMES, TLSPACK_EXE, cfg_name, c_index, c_index);

            printf ("\n%s", cmd_str);
            fflush (NULL);
            system (cmd_str);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        //launch client containers
        c_list = cfg_json["client"]["containers"];
        c_index = 0;
        for (auto it = c_list.begin(); it != c_list.end(); ++it)
        {
            sprintf (cmd_str,
                    "ssh -i %s.ssh/id_rsa -tt "
                    "-o LogLevel=quiet "
                    "-o StrictHostKeyChecking=no "
                    "-o UserKnownHostsFile=/dev/null "
                    "%s@%s "
                    "sudo docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=bridge --privileged "
                    "--name tlspack_client_%d --rm -it -d %s tgen %s client %s %d tlspack_client_%d config_topology",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    c_index, RUN_VOLUMES, TLSPACK_EXE, cfg_name, c_index, c_index);
                    
            printf ("\n%s", cmd_str);
            fflush (NULL);
            system (cmd_str);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        while (1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    else
    {   
        int c_index = atoi (argv[3]);
        char* c_name = argv[4];
        
        const char* toplogy_falg = "skip_topology";
        if (argc > 5)
        {
            toplogy_falg = argv[5];
        }
        
        if ( strcmp(toplogy_falg, "config_topology") == 0 )
        {
            auto topology = cfg_json[mode]["topology"].get<std::string>();
            if ( strcmp(topology.c_str(), "single-interface") == 0 )
            {
                auto macvlan = cfg_json[mode]["macvlan"].get<std::string>();
                sprintf (cmd_str,
                        "ssh -i %s.ssh/id_rsa -tt "
                        "-o LogLevel=quiet "
                        "-o StrictHostKeyChecking=no "
                        "-o UserKnownHostsFile=/dev/null "
                        "%s@%s "
                        "sudo docker network connect %s %s",
                        RUN_DIR_PATH,
                        ssh_user.c_str(), ssh_host.c_str(),
                        macvlan.c_str(), c_name);
                system (cmd_str);

                auto iface = cfg_json[mode]["iface"].get<std::string>();
                sprintf (cmd_str,
                        "ip link set dev %s up",
                        iface.c_str());
                system (cmd_str);

                sprintf (cmd_str,
                        "ip route add default dev %s table 200",
                        iface.c_str());
                system (cmd_str);


                auto subnets = cfg_json[mode]["containers"][c_index]["subnets"];
                for (auto it = subnets.begin(); it != subnets.end(); ++it)
                {
                    auto subnet = it.value().get<std::string>();
                    sprintf (cmd_str,
                            "ip -4 route add local %s dev lo",
                            subnet.c_str());
                    system (cmd_str);

                    sprintf (cmd_str,
                            "ip rule add from %s table 200",
                            subnet.c_str());
                    system (cmd_str);
                }
            }
        }

        signal(SIGPIPE, SIG_IGN);

        ev_sockstats all_ev_sockstats;
        tls_server_stats all_tls_server_stats;

        std::vector<ev_app*> app_list;
        auto apps = cfg_json[mode]["containers"][c_index]["apps"];
        int a_index = -1;
        for (auto it = apps.begin(); it != apps.end(); ++it)
        {
            auto app = it.value();
            const char* app_type = app["app_type"].get<std::string>().c_str();

            ev_app* next_app = nullptr;
            a_index++;
            if ( strcmp("tls_server", app_type) == 0 )
            {
                next_app = new tls_server_app (cfg_json
                                                , c_index
                                                , a_index
                                                , &all_tls_server_stats
                                                , &all_ev_sockstats);
            }
            else if ( strcmp("tls_client", app_type) == 0 )
            {
                next_app = new tls_client_app (cfg_json, c_index, a_index);
            }

            if (next_app)
            {
                app_list.push_back (next_app);
            }           
        }
        
        if ( not app_list.empty() )
        {
            while (1)
            {
                for (ev_app* app_ptr : app_list)
                {
                    app_ptr->run_iter ();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    }

    return 0;
}
