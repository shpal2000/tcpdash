#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "iovents.h"

static IoVentMethods_t* IOVMethods;
static IoVentOptions_t* IOVOptions;

void InitIoVents (IoVentMethods_t* methods
                    , IoVentOptions_t* options)
{
    IOVMethods = methods;
    IOVOptions = options;

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    
}

