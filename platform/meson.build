platform_src = ['tcp.c'
                , 'tls.c'
                , 'poll.c'
                , 'utils.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('..')

platform_lib = static_library('platform'
                , sources :platform_src
                , include_directories : incdir
                , dependencies : glib_dep)