#pragma once

#include <string>
#include <vector>
#include <sys/epoll.h>

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    void run();
private:
    int port;
    int server_fd;
    int epoll_fd;

    void setup_server();
    void handle_new_connection();
    void handle_client_data(int client_fd);
    void send_response(int client_fd, const std::string& response);
    std::string process_request(const std::string& request);
};