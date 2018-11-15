api_src = ['tcp.c'
                , 'tls.c'
                , 'poll.c'
                , 'utils.c']

glib = dependency('glib-2.0')
openssl = dependency('openssl')

incdir = include_directories('..')

api_lib = static_library('api'
                , sources :api_src
                , include_directories : incdir
                , dependencies : [glib, openssl])