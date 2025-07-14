#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>
#include <stdexcept>
#include <algorithm>

#include "client/tcp_client.hpp"
#include "server/calculator.hpp"

#define MAX_EVENTS 1024

TcpClient::TcpClient(const ClientConfig& config)
    : config(config),
      epoll_fd(-1),
      rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
    setup_epoll();
}

TcpClient::~TcpClient() {
    for (auto& conn : connections) {
        if (conn.fd != -1) {
            close(conn.fd);
        }
    }
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
}

void TcpClient::setup_epoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

void TcpClient::run() {
    create_connections();

    epoll_event events[MAX_EVENTS];
    int active_connections = config.connections;

    while (active_connections > 0) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i) {
            handle_connection_event(events[i].data.fd, events[i].events);
        }

        for (auto& conn : connections) {
            if (conn.fd != -1 && conn.sent_complete && conn.received_complete) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn.fd, nullptr);
                close(conn.fd);
                conn.fd = -1;
                active_connections--;
            }
        }
    }
}

void TcpClient::create_connections() {
    connections.reserve(config.connections);

    for (int i = 0; i < config.connections; ++i) {
        ConnectionState conn;
        conn.fd = create_socket_connection();
        conn.expression = generate_expression();
        conn.fragments = split_expression(conn.expression);
        conn.current_fragment = 0;
        conn.expected_result = calculate_expected_result(conn.expression);
        conn.sent_complete = false;
        conn.received_complete = false;

        connections.push_back(conn);

        epoll_event event {};
        event.events = EPOLLOUT | EPOLLET;
        event.data.fd = conn.fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn.fd, &event);
    }
}

std::string TcpClient::generate_expression() {
    std::uniform_int_distribution<int> num_dist(1, 100);
    std::uniform_int_distribution<int> op_dist(0, 3);

    std::ostringstream oss;

    oss << num_dist(rng);

    for (int i = 1; i < config.numbers; ++i) {
        char ops[] = {'+', '-', '*', '/'};
        oss << ops[op_dist(rng)] << num_dist(rng);
    }

    return oss.str();
}

std::vector<std::string> TcpClient::split_expression(const std::string& expr) {
    std::vector<std::string> fragments;

    if (expr.empty()) {
        return fragments;
    }

    std::uniform_int_distribution<size_t> frag_dist(
        1, std::max(1, static_cast<int>(expr.length()))
    );
    size_t num_fragments = frag_dist(rng);

    if (num_fragments == 1) {
        fragments.push_back(expr);
        return fragments;
    }

    std::vector<size_t> cuts;
    std::uniform_int_distribution<size_t> pos_dist(1, expr.length() - 1);

    for (size_t i = 0; i < num_fragments - 1; ++i) {
        cuts.push_back(pos_dist(rng));
    }

    std::sort(cuts.begin(), cuts.end());
    cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());

    size_t start = 0;
    for (size_t cut : cuts) {
        if (cut > start) {
            fragments.push_back(expr.substr(start, cut - start));
            start = cut;
        }
    }

    if (start < expr.length()) {
        fragments.push_back(expr.substr(start));
    }

    return fragments;
}

long long TcpClient::calculate_expected_result(const std::string& expr) {
    try {
        return Calculator::evaluate(expr);
    } catch (const std::exception&) {
        return 0;
    }
}

void TcpClient::handle_connection_event(int fd, uint32_t events) {
    auto it = std::find_if(
        connections.begin(),
        connections.end(),
        [fd](const ConnectionState& conn) { return conn.fd == fd; }
    );

    if (it == connections.end()) {
        return;
    }

    ConnectionState& conn = *it;

    if (events & EPOLLOUT && !conn.sent_complete) {
        send_next_fragment(conn);
    }

    if (events & EPOLLIN && !conn.received_complete) {
        handle_response(conn);
    }

    if (events & (EPOLLHUP | EPOLLERR)) {
        std::cerr << "Connection error for expression: " << conn.expression
                  << std::endl;
        conn.sent_complete = true;
        conn.received_complete = true;
    }
}

void TcpClient::send_next_fragment(ConnectionState& conn) {
    if (conn.current_fragment >= conn.fragments.size()) {
        conn.sent_complete = true;

        epoll_event event {};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = conn.fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn.fd, &event);
        return;
    }

    const std::string& fragment = conn.fragments[conn.current_fragment];
    ssize_t bytes_sent = write(conn.fd, fragment.c_str(), fragment.length());

    if (bytes_sent > 0) {
        conn.current_fragment++;

        if (conn.current_fragment >= conn.fragments.size()) {
            write(conn.fd, "\n", 1);
        }
    }
}

void TcpClient::handle_response(ConnectionState& conn) {
    char buffer[1024];
    ssize_t bytes_read = read(conn.fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        conn.received_data += buffer;

        if (conn.received_data.find('\n') != std::string::npos) {
            conn.received_complete = true;
            verify_result(conn);
        }
    } else if (bytes_read == 0) {
        conn.received_complete = true;
        verify_result(conn);
    }
}

void TcpClient::verify_result(const ConnectionState& conn) {
    std::istringstream iss(conn.received_data);
    std::string result_str;

    if (iss >> result_str) {
        if (result_str == "ERROR") {
            std::cerr << "Expression: " << conn.expression
                      << " | Server response: ERROR"
                      << " | Expected: " << conn.expected_result << std::endl;
        } else {
            try {
                long long server_result = std::stoll(result_str);
                if (server_result != conn.expected_result) {
                    std::cerr << "Expression: " << conn.expression
                              << " | Server response: " << server_result
                              << " | Expected: " << conn.expected_result
                              << std::endl;
                }
            } catch (const std::exception&) {
                std::cerr << "Expression: " << conn.expression
                          << " | Server response: " << result_str
                          << " | Expected: " << conn.expected_result
                          << std::endl;
            }
        }
    }
}

int TcpClient::create_socket_connection() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config.server_port);

    if (inet_pton(AF_INET, config.server_addr.c_str(), &server_addr.sin_addr) <=
        0) {
        close(sock);
        throw std::runtime_error("Invalid server address");
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) ==
        -1) {
        close(sock);
        throw std::runtime_error("Failed to connect to server");
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);

    return sock;
}