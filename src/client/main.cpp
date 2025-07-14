#include <iostream>
#include <stdexcept>
#include "client/tcp_client.hpp"

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name
              << " <n> <connections> <server_addr> <server_port>" << std::endl;
    std::cerr << "  n           - количество чисел в выражении" << std::endl;
    std::cerr << "  connections - количество TCP сессий к серверу" << std::endl;
    std::cerr << "  server_addr - адрес TCP сервера" << std::endl;
    std::cerr << "  server_port - порт TCP сервера" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        ClientConfig config;
        config.numbers = std::stoi(argv[1]);
        config.connections = std::stoi(argv[2]);
        config.server_addr = argv[3];
        config.server_port = std::stoi(argv[4]);

        if (config.numbers <= 0) {
            std::cerr << "Error: n must be positive" << std::endl;
            return 1;
        }

        if (config.connections <= 0) {
            std::cerr << "Error: connections must be positive" << std::endl;
            return 1;
        }

        if (config.server_port <= 0 || config.server_port > 65535) {
            std::cerr << "Error: server_port must be between 0 and 65534"
                      << std::endl;
            return 1;
        }

        TcpClient client(config);
        client.run();

    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}