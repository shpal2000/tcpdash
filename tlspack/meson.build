project('tlspack', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++14'])
openssl = dependency('openssl')

nlohmann_json_proj = subproject('nlohmann_json')
nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')

executable('tlspack.exe'
            , sources : ['ev_socket.cpp'
                          , 'ev_app.cpp'
                          , './apps/tls_server/tlssrv_app.cpp'
                          , './apps/tls_server/tlssrv_socket.cpp'
                          , 'main.cpp' ]
            , dependencies: [openssl, nlohmann_json_dep]
            , install : true)

