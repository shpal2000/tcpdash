#ifndef __TLSPACK_PLATFORM_H
#define __TLSPACK_PLATFORM_H

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <iostream>
#include <fstream>
#include<sstream>
#include <queue>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <random>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

template<typename P, typename C>
inline bool isclass(const C*) {
   return std::is_base_of<P, C>::value;
}

enum enum_close_type { close_fin=1, close_reset };

enum enum_close_notify { close_notify_no_send=1
                           , close_notify_send
                           ,  close_notify_send_recv};
                           
enum enum_tls_version { sslv3=0, tls1, tls1_1, tls1_2, tls1_3, tls_all};

#endif