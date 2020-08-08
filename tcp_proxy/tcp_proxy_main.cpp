#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>
#include <chrono>

#include "tcp_proxy.hpp"


#define MAX_CONFIG_DIR_PATH 256
#define MAX_CONFIG_FILE_PATH 512
#define MAX_EXE_FILE_PATH 512
#define MAX_VOLUME_STRING 1024
#define MAX_CMD_LEN 2048
#define MAX_CMD_LEN 2048
#define MAX_RESULT_DIR_PATH 2048
#define MAX_FILE_PATH_LEN 2048
#define RUN_DIR_PATH "/rundir/"
#define SRC_DIR_PATH "/tcp_proxy/"
#define MAX_REGISTRY_DIR_PATH 2048
#define MAX_ZONE_CNAME 64
char cmd_str [MAX_CMD_LEN];
char cfg_dir [MAX_CONFIG_DIR_PATH];
char cfg_file [MAX_CONFIG_FILE_PATH];
char ssh_host_file [MAX_CONFIG_FILE_PATH];
char volume_string [MAX_VOLUME_STRING];
char result_dir [MAX_RESULT_DIR_PATH];
char curr_dir_file [MAX_FILE_PATH_LEN];
char zone_file [MAX_FILE_PATH_LEN];
char registry_dir [MAX_REGISTRY_DIR_PATH];
char zone_cname [MAX_ZONE_CNAME];

tp_stats* zone_tp_stats = nullptr;

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
    if ( strcmp(topology.c_str(), "proxy-interface") == 0 )
    {
        //ta_iface: get proxy interfaces
        auto ta_macvlan = cfg_json["zones"][z_index]["ta_macvlan"].get<std::string>();
        auto ta_iface = cfg_json["zones"][z_index]["ta_iface"].get<std::string>();
        auto ta_mac = cfg_json["zones"][z_index]["ta_mac"].get<std::string>();
        auto ta_ip = cfg_json["zones"][z_index]["ta_ip"].get<std::string>();
        auto ta_net = cfg_json["zones"][z_index]["ta_net"].get<std::string>();
        auto ta_netmask = cfg_json["zones"][z_index]["ta_netmask"].get<std::string>();
        auto ta_gateway = cfg_json["zones"][z_index]["ta_gateway"].get<std::string>();

        //tb_iface: get proxy interfaces
        auto tb_macvlan = cfg_json["zones"][z_index]["tb_macvlan"].get<std::string>();
        auto tb_iface = cfg_json["zones"][z_index]["tb_iface"].get<std::string>();
        auto tb_mac = cfg_json["zones"][z_index]["tb_mac"].get<std::string>();
        auto tb_ip = cfg_json["zones"][z_index]["tb_ip"].get<std::string>();
        auto tb_net = cfg_json["zones"][z_index]["tb_net"].get<std::string>();
        auto tb_netmask = cfg_json["zones"][z_index]["tb_netmask"].get<std::string>();
        auto tb_gateway = cfg_json["zones"][z_index]["tb_gateway"].get<std::string>();

        auto proxy_traffic_port = cfg_json["zones"][z_index]["proxy_traffic_port"].get<std::string>();
        auto proxy_app_port = cfg_json["zones"][z_index]["proxy_app_port"].get<std::string>();


        //connect to docker network
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker network connect %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_macvlan.c_str(), zone_cname);
        system_cmd ("connect docker network", cmd_str);

        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo docker network connect %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_macvlan.c_str(), zone_cname);
        system_cmd ("connect docker network", cmd_str);




        //set ip address
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ifconfig %s %s up",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_iface.c_str(), ta_ip.c_str());
        system_cmd ("set ip address", cmd_str);

        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ifconfig %s %s up",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_iface.c_str(), tb_ip.c_str());
        system_cmd ("set ip address", cmd_str);




        //set mac address
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ifconfig %s hw ether %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_iface.c_str(), ta_mac.c_str());
        system_cmd ("set mac address", cmd_str);

        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ifconfig %s hw ether %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_iface.c_str(), tb_mac.c_str());
        system_cmd ("set mac address", cmd_str);




        //static mac address for the gateway
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo arp -i %s -s %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_iface.c_str(), ta_gateway.c_str(), tb_mac.c_str());
        system_cmd ("static mac address for the gateway", cmd_str);

        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo arp -i %s -s %s %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_iface.c_str(), tb_gateway.c_str(), ta_mac.c_str());
        system_cmd ("static mac address for the gateway", cmd_str);




        //rp_filter set to 0
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo sysctl net.ipv4.conf.%s.rp_filter=0",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_iface.c_str());
        system_cmd ("rp_filter set to 0", cmd_str);

        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo sysctl net.ipv4.conf.%s.rp_filter=0",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_iface.c_str());
        system_cmd ("rp_filter set to 0", cmd_str);


        //set rp filter rule: 1
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo sysctl net.ipv4.conf.all.rp_filter=0",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set rp filter rule: 1", cmd_str);

        //set rp filter rule: 2
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo sysctl net.ipv4.conf.default.rp_filter=0",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set rp filter rule: 2", cmd_str);

        //set iptable rule: 1
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -N DIVERT",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set iptable rule: 1", cmd_str);

        //set iptable rule: 2
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -A PREROUTING -p tcp -m socket -j DIVERT",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set iptable rule: 2", cmd_str);

        //set iptable rule: 3
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -A DIVERT -j MARK --set-mark 1",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set iptable rule: 3", cmd_str);

        //set iptable rule: 4
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -A DIVERT -j ACCEPT",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set iptable rule: 4", cmd_str);

        //set ip rule: 5
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ip rule add fwmark 1 lookup 100",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set ip rule: 5", cmd_str);

        //set ip rule: 6
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo ip route add local 0.0.0.0/0 dev lo table 100",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str());
        system_cmd ("set ip rule: 6", cmd_str);

        //set iptable rule: 7
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -A PREROUTING -i %s -p tcp --dport %s -j TPROXY --tproxy-mark 0x1/0x1 --on-port %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_iface.c_str(), proxy_traffic_port.c_str(), proxy_app_port.c_str());
        system_cmd ("set iptable rule: 7", cmd_str);

        //set iptable rule: 8
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo iptables -t mangle -A PREROUTING -i %s -p tcp --dport %s -j TPROXY --tproxy-mark 0x1/0x1 --on-port %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_iface.c_str(), proxy_traffic_port.c_str(), proxy_app_port.c_str());
        system_cmd ("set iptable rule: 8", cmd_str);

        //set route rule: 9
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo route add-net %s netmask %s gw %s dev %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                ta_net.c_str(), ta_netmask.c_str(), ta_gateway.c_str(), ta_iface.c_str());
        system_cmd ("set route rule: 9", cmd_str);

        //set route rule: 10
        sprintf (cmd_str,
                "ssh -i %ssys/id_rsa -tt "
                // "-o LogLevel=quiet "
                "-o StrictHostKeyChecking=no "
                "-o UserKnownHostsFile=/dev/null "
                "%s@%s "
                "sudo route add-net %s netmask %s gw %s dev %s",
                RUN_DIR_PATH,
                ssh_user.c_str(), ssh_host.c_str(),
                tb_net.c_str(), tb_netmask.c_str(), tb_gateway.c_str(), tb_iface.c_str());
        system_cmd ("set route rule: 10", cmd_str);
    }
    else
    {   
        printf ("unknown zone type %s\n", topology.c_str());
        exit (-1);
    }

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

static tp_app* create_app (json cfg_json, int z_index)
{
    auto app_json = cfg_json["zones"][z_index];

    zone_tp_stats = new tp_stats();
    tp_app* the_app = new tp_app (app_json, zone_tp_stats);

    return the_app;
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

    }
    else if (strcmp(mode, "stop") == 0)
    {

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

        sprintf (result_dir, "%straffic/%s/%s/%s/", RUN_DIR_PATH, cfg_name
                                                    , "results", run_tag);

        if ( strcmp(config_zone_flag, "config_zone") == 0 )
        {
            config_zone (cfg_json, cfg_name, z_index);
        }

        signal(SIGPIPE, SIG_IGN);

        //tcpdump:ta
        sprintf (curr_dir_file,
                "%s%s/"
                "ta.pcap",
                result_dir, zone_label.c_str());
        auto ta_iface = cfg_json["zones"][z_index]["ta_iface"].get<std::string>();
        auto ta_tcpdump_s = cfg_json["zones"][z_index]["ta_tcpdump"].get<std::string>();
        sprintf (cmd_str,
            "tcpdump -i %s -n %s -w %s&",
                ta_iface.c_str(), ta_tcpdump_s.c_str(), curr_dir_file);
        system_cmd ("tcpdump start", cmd_str);

        //tcpdump:tb
        sprintf (curr_dir_file,
                "%s%s/"
                "tb.pcap",
                result_dir, zone_label.c_str());
        auto tb_iface = cfg_json["zones"][z_index]["tb_iface"].get<std::string>();
        auto tb_tcpdump_s = cfg_json["zones"][z_index]["tb_tcpdump"].get<std::string>();
        sprintf (cmd_str,
            "tcpdump -i %s -n %s -w %s&",
                tb_iface.c_str(), tb_tcpdump_s.c_str(), curr_dir_file);
        system_cmd ("tcpdump start", cmd_str);

        std::this_thread::sleep_for(std::chrono::seconds(8));

        tp_app* next_app = create_app (cfg_json, z_index);

        update_registry_state (zone_file, 1);

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

            next_app->run_iter (is_tick_sec);

            if (is_tick_sec)
            {
                zone_tp_stats->tick_sec();
                is_tick_sec = false;
            }

            if (tick_5sec == 5)
            {
                tick_5sec = 0;

                sprintf (curr_dir_file,
                        "%s%s/"
                        "ev_sockstats.json",
                        result_dir, zone_label.c_str());
                dump_stats (curr_dir_file, zone_tp_stats);
            }
        }
    }

    return 0;
}

