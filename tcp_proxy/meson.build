project('tcp_proxy', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++17'])

openssl = dependency('openssl')
nlohmann_json_proj = subproject('json')
nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')
#ev_sock = dependency('ev_sock', static: true)
incdirs = include_directories(['../ev_sock/'])

executable('tcp_proxy.exe'
            , sources : [ '../ev_sock/ev_socket.cpp'
                          , '../ev_sock/ev_app.cpp'
                          , 'tcp_proxy.cpp'
                          , 'tcp_proxy_main.cpp' ]
            , dependencies: [openssl, nlohmann_json_dep]
            , include_directories : [incdirs]
            , install : true)

