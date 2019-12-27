#include <stdio.h>
#include <iostream>
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
    json cfg_json = json::parse(cfg_str);

    auto iface = cfg_json["server"]["iface"].get<std::string>();

    tlssrv_app* app = new tlssrv_app ();
    printf ("%s\n", argv[argc*0]);
    while (1)
    {
        app->run_iter ();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}
