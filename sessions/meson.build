sessions_src = ['tcp_connect.c'
                , 'tls_connect.c'
                , 'sessions.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories('..')

sessionslib = static_library('sessions'
                , sources : sessions_src
                , include_directories : incdir
                , dependencies : glib_dep)