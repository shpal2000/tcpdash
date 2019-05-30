tls_client_src = ['tls_client.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../../platform2/'
                                , '../../iovents2/'
                                , '../../msg_io2/'
                                , '../../nadmin/'
                                , '../engine/'])

tls_client_lib = static_library('tls_client'
                , sources :tls_client_src
                , include_directories : incdir
                , dependencies : [glib, json_glib_dep, openssl])