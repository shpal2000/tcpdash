tcp_client_src = ['tcp_client.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('../..')

executable('tcp_client.exe'
            , sources : tcp_client_src
            , include_directories : incdir
            , dependencies : glib_dep
            , link_with : sessionslib)