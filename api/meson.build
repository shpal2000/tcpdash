api_src = ['tcp.c'
                , 'tls.c'
                , 'poll.c'
                , 'utils.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('..')

api_lib = static_library('api'
                , sources :api_src
                , include_directories : incdir
                , dependencies : glib_dep)