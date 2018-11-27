iovent_src = ['iovents.c']

glib = dependency('glib-2.0')
openssl = dependency('openssl')

incdir = include_directories('../..')

iovent_lib = static_library('iovent'
                , sources :iovent_src
                , include_directories : incdir
                , dependencies : [glib, openssl])