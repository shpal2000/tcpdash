tcp_proxy_src = ['tcp_proxy.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../..'
                                , '../../iovents/'
                                , '../../msg_io/'
                                , '../../nadmin/'])

executable('tcp_proxy.exe'
                    , sources : tcp_proxy_src
                    , include_directories : incdir
                    , dependencies : [glib_dep, json_glib_dep]
                    , link_with : [platform_lib
                                    , iovent_lib
                                    , msg_io_lib])

