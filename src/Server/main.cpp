#include <iostream>
#include <asio.hpp>

#include "TCPServer.hpp"

int main(int, char**) {
    try {
        asio::io_context io_context;
        TCPServer server(io_context);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
