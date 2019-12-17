project('tlspack', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++14'])
openssl = dependency('openssl')

executable('tlspack.exe'
            , sources : ['ev_socket.cpp'
                          , 'ev_app.cpp'
                          , './apps/tls_server/tlssrv_app.cpp'
                          , './apps/tls_server/tlssrv_socket.cpp'
                          , 'tlspack_main.cpp' ]
            , dependencies: [openssl]
            , install : true)
