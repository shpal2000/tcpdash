engine_src = ['engine.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories(['../../platform2/'
                                , '../../iovents2/'
                                , '../../msg_io2/'
                                , '../../nadmin/'])

executable('engine.exe'
            , sources : engine_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [platform2_lib
                            , iovent2_lib
                            , msg_io2_lib])
