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
#include <queue>
#include <cstring>
#include <thread>
#include <chrono>
#include <map>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

template<typename P, typename C>
inline bool isclass(const C*) {
   return std::is_base_of<P, C>::value;
}
#endif