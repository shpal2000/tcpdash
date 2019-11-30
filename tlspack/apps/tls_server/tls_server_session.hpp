#include "tls_server_connection.hpp"

class tls_server_session
{
public:
    tls_server_session(/* args */);
    ~tls_server_session();

private:
    tls_server_connection* m_connection;

};

tls_server_session::tls_server_session(/* args */)
{
}

tls_server_session::~tls_server_session()
{
}
