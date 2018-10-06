#ifndef __UTILS_H
#define __UTILS_H

#include <gmodule.h>

#define CreateArray(__type,__count) g_new(__type, __count)
#define CreateStruct0(__type) g_slice_new0(__type) 

#endif