project('tcp_proxy', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++17'])

nlohmann_json_proj = subproject('json')
nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')
ev_sock = dependency('ev_sock', static: true)

executable('tcp_proxy.exe'
            , sources : ['tcp_proxy.cpp'
                          , 'main.cpp' ]
            , dependencies: [nlohmann_json_dep, ev_sock]
            , install : true)

