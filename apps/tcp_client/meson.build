tcp_client_src = ['tcp_client.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../../linux/'
                                , '../../iovents/'
                                , '../../msg_io/'
                                , '../../nadmin/'])

executable('tcp_client.exe'
            , sources : tcp_client_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [platform_lib
                            , iovent_lib
                            , msg_io_lib])
