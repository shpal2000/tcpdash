tcp_client_src = ['tls_sample_client.c']

glib_dep = dependency('glib-2.0')

incdir = include_directories(['../..', '../../iovents/v1/'])

tcp_client_lib =  static_library('tcp_client'
                    , sources : tcp_client_src
                    , include_directories : incdir
                    , dependencies : glib_dep
                    , link_with : api_lib)

