msg_io_src = ['msg_io.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../platform/'
                                , '../iovents/'])

msg_io_lib =  static_library('msg_io'
                    , sources : msg_io_src
                    , include_directories : incdir
                    , dependencies : [glib_dep, json_glib_dep]
                    , link_with : [platform_lib, iovent_lib]
            )

