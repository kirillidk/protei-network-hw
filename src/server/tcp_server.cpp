#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>

#include "server/tcp_server.hpp"
#include "server/calculator.hpp"

#define MAX_EVENTS 1024

TcpServer::TcpServer(int port) : port(port), server_fd(-1), epoll_fd(-1) {
    setup_server();
}

TcpServer::~TcpServer() {
    if (epoll_fd != -1) close(epoll_fd);
    if (server_fd != -1) close(server_fd);
}

void TcpServer::setup_server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, SOMAXCONN);

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    epoll_fd = epoll_create1(0);
    epoll_event event {};
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
}

void TcpServer::run() {
    std::cout << "Server listening on port " << port << std::endl;

    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                handle_new_connection();
            } else {
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void TcpServer::handle_new_connection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) return;

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    epoll_event event {};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
}

void TcpServer::handle_client_data(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read <= 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';
    std::string request(buffer);

    std::string response = process_request(request);
    send_response(client_fd, response);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
}

void TcpServer::send_response(int client_fd, const std::string& response) {
    write(client_fd, response.c_str(), response.length());
}

std::string TcpServer::process_request(const std::string& request) {
    std::istringstream iss(request);
    std::string expression;
    std::string result;

    while (iss >> expression) {
        try {
            long long value = Calculator::evaluate(expression);
            result += std::to_string(value) + " ";
        } catch (const std::exception& e) {
            result += "ERROR ";
        }
    }

    return result + "\n";
}