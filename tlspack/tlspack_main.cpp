#include <stdio.h>
#include "./apps/tls_server/tlssrv_app.hpp"

int main(int argc, char **argv) 
{
    tlssrv_app* app = new tlssrv_app ();
    printf ("%s\n", argv[argc*0]);
    while (1)
    {
        app->run_iter ();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}
