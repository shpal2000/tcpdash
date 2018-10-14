tcp_client_src = ['tcp_client.c']

tcp_server_src = ['tcp_server.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('../..')

tcp_client_lib =  static_library('tcp_client'
                    , sources : tcp_client_src
                    , include_directories : incdir
                    , dependencies : glib_dep
                    , link_with : sessionslib)

executable('tcp_server.exe'
                    , sources : tcp_server_src
                    , include_directories : incdir
                    , dependencies : glib_dep
                    , link_with : sessionslib)
