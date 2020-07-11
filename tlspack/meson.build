project('tlspack', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++17'])

#add_project_arguments('-DOPENSSL_1_1_1=1', language : 'cpp')
#export PKG_CONFIG_PATH="/root/openssl-1.0.2u/local/lib/pkgconfig"
#export PKG_CONFIG_PATH="/root/openssl-1.1.0l/local/lib/pkgconfig"
#export PKG_CONFIG_PATH="/root/openssl-1.1.1d/local/lib/pkgconfig"

openssl = dependency('openssl')
nlohmann_json_proj = subproject('json')
nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')
ev_sock = dependency('ev_sock', static: true)

executable('tlspack.exe'
            , sources : ['./apps/tls_server/tls_server.cpp'
                          , './apps/tls_client/tls_client.cpp'
                          , 'main.cpp' ]
            , dependencies: [openssl, nlohmann_json_dep, ev_sock]
            , install : true)

