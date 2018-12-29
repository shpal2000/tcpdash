tcp_load_src = ['tcp_client.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories(['../..', '../../iovents/v1/'])

tcp_load_lib =  static_library('tcp_load'
                    , sources : tcp_load_src
                    , include_directories : incdir
                    , dependencies : glib_dep
                    , link_with : [api_lib
                                    , iovent_lib])

