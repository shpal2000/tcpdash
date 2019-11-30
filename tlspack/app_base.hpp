#include "socket_manager.hpp"

class app_base
{
public:
    app_base(/* args */);
    ~app_base();

    virtual int run_loop() = 0;

private:
    socket_manager* m_socket_manager;
};

