tcp_proxy_src = ['tcp_proxy.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../..', '../../iovents/v1/'])

tcp_proxy_lib =  static_library('tcp_proxy'
                    , sources : tcp_proxy_src
                    , include_directories : incdir
                    , dependencies : [glib_dep, json_glib_dep]
                    , link_with : [api_lib
                                    , iovent_lib])

