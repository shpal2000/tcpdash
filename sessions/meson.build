sessions_src = ['tcp_connect.c', 'tls_connect.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('..')

static_library('sessions.a'
                , sources : sessions_src
                , include_directories : incdir
                , dependencies : glib_dep)