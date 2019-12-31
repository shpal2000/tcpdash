#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>
#include "./apps/tls_server/tlssrv_app.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

auto cfg_str = R"({
    "server" : {
        "iface" : "eth2",
        "containers" : [
            {
                "app_type" : "tlssrv",
                "subnets" : ["12.20.60.0/24"],
                "max_act_sess" : 10000,
                "max_err_sess" : 10000,
                "srv_list" : [
                    {
                        "srv_ip" : "12.20.60.1",
                        "srv_port" : 8443,
                        "cs_data_len" : 10,
                        "sc_data_len" : 20,
                        "cs_start_tls_len" : 0,
                        "sc_start_tls_len" : 0
                    }
                ]
            }
        ]
    },

    "client" : {
	    "iface" : "eth1",
        "containers" : [
            {
                "app_type" : "tlsclnt",
	            "subnets" : ["12.20.50.0/24"],
                "conn_per_sec" : 85,
                "max_act_sess" : 10000,
                "max_err_sess" : 10000,
                "max_total_sess" : 10,
                "cs_grp_list" : [
                    {
                        "srv_ip"   : "12.20.60.1",
                        "srv_port" : 8443,

                        "clnt_ip_begin" : "12.20.50.1",
                        "clnt_ip_end" : "12.20.50.100",
                        "clnt_port_begin" : 10000,
                        "clnt_port_end" : 19999,

                        "cs_data_len" : 10,
                        "sc_data_len" : 20,
                        "cs_start_tls_len" : 0,
                        "sc_start_tls_len" : 0
                    }
                ]
            }
        ]
    }
})";

int main(int argc, char **argv) 
{
    signal(SIGPIPE, SIG_IGN);

    //sudo docker run tgen --rm -it -d tlspack.exe config try1

    //sudo docker run tgen --rm -it -d tlspack.exe 0 try1
    //sudo docker run tgen --rm -it -d tlspack.exe 1 try1
    //sudo docker run tgen --rm -it -d tlspack.exe 2 try1
    //sudo docker run tgen --rm -it -d tlspack.exe 3 try1
    //sudo docker run tgen --rm -it -d tlspack.exe 4 try1

    char* mode = argv[1];

    if ( strcmp(mode, "config") == 0)
    {
        char* cfg_name = argv[2];
        char cfg_dir [256];
        char cfg_file [256];
        sprintf (cfg_dir, "%s%s/", "/rundir/", cfg_name);
        sprintf (cfg_file, "%s%s", cfg_dir, "config.json");
        
        std::ifstream cfg_stream(cfg_file);
        json cfg_json = json::parse(cfg_stream);

        // launch containers
        // system ("ssh -i /rundir/.ssh/id_rsa -tt -o StrictHostKeyChecking=no shirish@104.211.35.20 sudo docker network connect eth0macvlan tgen_s");

        while (1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } 
    else
    {
        // char* cfg_file = argv[1];
        // int container_index = atoi(argv[2]);
        // std::ifstream cfg_file(cfg_file);

    
        // // json cfg_json = json::parse(cfg_file);

        // // auto iface = cfg_json["server"]["iface"].get<std::string>();

    
        tlssrv_app* app = new tlssrv_app ();
        printf ("%s\n", argv[argc*0]);
        while (1)
        {
            app->run_iter ();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    return 0;
}
