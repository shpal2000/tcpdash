platform_src = ['tcp.c'
                , 'tls.c'
                , 'poll.c'
                , 'utils.c']

glib = dependency('glib-2.0')
openssl = dependency('openssl')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories('../')

platform2_lib = static_library('platform2'
                , sources :platform_src
                , include_directories : incdir
                , dependencies : [glib, json_glib_dep, openssl])