project('tcpdash', 'c')

add_global_arguments('-std=gnu99', language : 'c')

subdir('platform')
subdir('lab')
subdir('iovents')
subdir('apps')

tcpdash_src = ['tcpdash.c']

glib_dep = dependency('glib-2.0')
json_glib_dep = dependency('json-glib-1.0')

incdir = include_directories('./')

executable('tcpdash.exe'
            , sources : tcpdash_src
            , include_directories : incdir
            , dependencies : [glib_dep, json_glib_dep]
            , link_with : [tls_sample_client_lib
                            , tls_sample_server_lib
                            , tcp_proxy_lib
                            , tcp_load_lib]) 


