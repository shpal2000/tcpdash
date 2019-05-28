iovent_src = ['iovents.c']

glib = dependency('glib-2.0')
openssl = dependency('openssl')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../platform2/'])
                                
iovent2_lib = static_library('iovent2'
                , sources :iovent_src
                , include_directories : incdir
                , dependencies : [glib, json_glib_dep, openssl])