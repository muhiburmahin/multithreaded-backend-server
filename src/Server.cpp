#include "Server.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port, ThreadPool& pool, RateLimiter& limiter)
    : port(port), pool(pool), limiter(limiter) {}

void Server::run() {
    signal(SIGPIPE, SIG_IGN);

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listenFd);
        listenFd = -1;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(listenFd);
        listenFd = -1;
        return;
    }

    if (listen(listenFd, 128) < 0) {
        perror("listen");
        close(listenFd);
        listenFd = -1;
        return;
    }

    std::cout << "Listening on port " << port << std::endl;

    while (running) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (clientFd < 0) {
            if (!running || errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        std::string ip = inet_ntoa(clientAddr.sin_addr);
        // Phase 1: handle inline (thread pool comes in Phase 3)
        handleConnection(clientFd, ip);
    }

    if (listenFd >= 0) {
        close(listenFd);
        listenFd = -1;
    }
}

void Server::stop() {
    running = false;
    if (listenFd >= 0) {
        close(listenFd);
        listenFd = -1;
    }
}

void Server::handleConnection(int clientFd, const std::string& /*ip*/) {
    char buf[1024];
    recv(clientFd, buf, sizeof(buf) - 1, 0);

    const char* reply =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "ok";

    send(clientFd, reply, std::strlen(reply), 0);
    close(clientFd);
}
