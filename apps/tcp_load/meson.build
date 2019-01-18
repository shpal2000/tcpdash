tcp_client_src = ['tcp_client.c']
tcp_server_src = ['tcp_server.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories(['../..', '../../iovents/v1/'])

executable('tcp_client.exe'
            , sources : tcp_client_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [api_lib
                            , iovent_lib
                            , msg_io_lib])

executable('tcp_server.exe'
            , sources : tcp_server_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [api_lib
                            , iovent_lib
                            , msg_io_lib])