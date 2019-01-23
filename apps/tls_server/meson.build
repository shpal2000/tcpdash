tls_server_src = ['tls_server.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../..'
                                , '../../iovents/v1/'
                                , '../../msg_io/'
                                , '../../nadmin/'])

executable('tls_server.exe'
            , sources : tls_server_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [platform_lib
                            , iovent_lib
                            , msg_io_lib])