project('ev_sock', 'cpp',
  version : '1.0',
  default_options : ['warning_level=3'
                      , 'cpp_std=c++17'])

openssl = dependency('openssl')

pkg = import('pkgconfig')

install_headers([ 'platform.hpp'
                  , 'ev_socket.hpp'
                  , 'ev_app.hpp']
                  , subdir : 'ev_sock')

ev_sock = static_library('ev_sock'
                    , sources : ['ev_socket.cpp'
                                  , 'ev_app.cpp']
                    , dependencies: [openssl]
                    , install : true)

pkg.generate(libraries : ev_sock
              , version : '1.0'
              , name : 'ev_sock'
              , subdirs: 'ev_sock'
              , description: 'ev_socket, ev_app library')
