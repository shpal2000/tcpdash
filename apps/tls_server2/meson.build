tls_server_src = ['tls_server.c']

glib_dep = dependency('glib-2.0')
openssl = dependency('openssl')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../../platform2/'
                                , '../../iovents2/'
                                , '../../msg_io2/'
                                , '../../nadmin/'
                                , '../engine/'])

tls_server_lib = static_library('tls_server'
                , sources :tls_server_src
                , include_directories : incdir
                , dependencies : [glib, json_glib_dep, openssl])