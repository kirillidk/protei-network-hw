#pragma once

#include <string>
#include <vector>
#include <random>
#include <sys/epoll.h>

struct ClientConfig {
    int numbers;
    int connections;
    std::string server_addr;
    int server_port;
};

class TcpClient {
public:
    explicit TcpClient(const ClientConfig& config);
    ~TcpClient();

    void run();
private:
    ClientConfig config;
    int epoll_fd;
    std::mt19937 rng;

    struct ConnectionState {
        int fd;
        std::string expression;
        std::vector<std::string> fragments;
        size_t current_fragment;
        std::string received_data;
        long long expected_result;
        bool sent_complete;
        bool received_complete;
    };

    std::vector<ConnectionState> connections;

    void setup_epoll();
    void create_connections();
    std::string generate_expression();
    std::vector<std::string> split_expression(const std::string& expr);
    long long calculate_expected_result(const std::string& expr);
    void handle_connection_event(int fd, uint32_t events);
    void send_next_fragment(ConnectionState& conn);
    void handle_response(ConnectionState& conn);
    void verify_result(const ConnectionState& conn);
    int create_socket_connection();
};