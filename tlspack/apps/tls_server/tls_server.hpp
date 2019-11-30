#include "app_base.hpp"

class tls_server : public app_base
{
public:
    tls_server(/* args */);
    ~tls_server();

    int run_loop();

private:
};
