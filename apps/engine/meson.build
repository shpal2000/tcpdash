engine_src = ['app_engine.c'
                , 'app_channel.c'
                , 'app_methods.c'
                , 'app_sockets.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../../platform2/'
                                , '../../iovents2/'
                                , '../../msg_io2/'
                                , '../../nadmin/'])

executable('app_engine.exe'
            , sources : engine_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [platform2_lib
                            , iovent2_lib
                            , msg_io2_lib
                            , tls_client_lib])
