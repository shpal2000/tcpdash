socks_src = ['tcp.c'
                , 'tls.c'
                , 'poll.c'
                , 'utils.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('..')

socks_lib = static_library('socks'
                , sources :socks_src
                , include_directories : incdir
                , dependencies : glib_dep)