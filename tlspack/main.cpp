#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include "./apps/tls_server/tlssrv_app.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define MAX_CONFIG_DIR_PATH 256
#define MAX_CONFIG_FILE_PATH 512
#define MAX_SSH_CMD_LEN 1024
#define RUN_DIR_PATH "/rundir/"
#define TLSPACK_EXE "/tgen_workspace/tcpdash/tlspack/build/tlspack.exe"
#define RUN_VOLUMES "--volume=/home/shirish/tgen_workspace:/tgen_workspace --volume=/home/shirish/rundir:/rundir"
char ssh_cmd [MAX_SSH_CMD_LEN];
char cfg_dir [MAX_CONFIG_DIR_PATH];
char cfg_file [MAX_CONFIG_FILE_PATH];



int main(int argc, char **argv) 
{
    char* mode = argv[1];
    char* cfg_name = argv[2];
    sprintf (cfg_dir, "%s%s/", RUN_DIR_PATH, cfg_name);
    sprintf (cfg_file, "%s%s", cfg_dir, "config.json");
    std::ifstream cfg_stream(cfg_file);
    json cfg_json = json::parse(cfg_stream);
    auto ssh_host = cfg_json["host"].get<std::string>();
    auto ssh_user = cfg_json["user"].get<std::string>();

    if (strcmp(mode, "run") == 0)
    {
        //launch server containers
        auto c_list = cfg_json["server"]["containers"];
        int c_index = 0;
        for (auto it = c_list.begin(); it != c_list.end(); ++it)
        {
            sprintf (ssh_cmd,
                    "ssh -i %s.ssh/id_rsa -tt -o StrictHostKeyChecking=no "
                    "%s@%s "
                    "sudo docker run --name tlspack_server_%d --rm -it -d %s tgen %s server %s %d tlspack_server_%d",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    c_index, RUN_VOLUMES, TLSPACK_EXE, cfg_name, c_index, c_index);

            printf ("\n%s", ssh_cmd);
            fflush (NULL);
            system (ssh_cmd);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        //launch client containers
        c_list = cfg_json["client"]["containers"];
        c_index = 0;
        for (auto it = c_list.begin(); it != c_list.end(); ++it)
        {
            sprintf (ssh_cmd,
                    "ssh -i %s.ssh/id_rsa -tt -o StrictHostKeyChecking=no "
                    "%s@%s "
                    "sudo docker run --name tlspack_client_%d --rm -it -d %s tgen %s client %s %d tlspack_client_%d",
                    RUN_DIR_PATH,
                    ssh_user.c_str(), ssh_host.c_str(),
                    c_index, RUN_VOLUMES, TLSPACK_EXE, cfg_name, c_index, c_index);
                    
            printf ("\n%s", ssh_cmd);
            fflush (NULL);
            system (ssh_cmd);
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
        auto iface = cfg_json[mode]["iface"].get<std::string>();

        
        signal(SIGPIPE, SIG_IGN);

        sprintf (ssh_cmd,
                "ssh -i %s.ssh/id_rsa -tt -o StrictHostKeyChecking=no "
                "%s@%s "
                "sudo docker network connect %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                iface.c_str(), c_name);

        system (ssh_cmd);

        while (1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            printf ("\n%s", ssh_cmd);
        }

        // tlssrv_app* app = new tlssrv_app ();
        // printf ("%s\n", argv[argc*0]);
        // while (1)
        // {
        //     app->run_iter ();
        //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // }
    }

    return 0;
}
