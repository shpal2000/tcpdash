#include <stdio.h>
#include "./apps/tls_server/tlssrv_app.hpp"

int main(int argc, char **argv) 
{
    tlssrv_app* app = new tlssrv_app ();
    app->run_iter ();
    printf ("%s\n", argv[argc*0]);
    return 0;
}
