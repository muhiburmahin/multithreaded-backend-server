#include "Server.hpp"

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Server::Server(int port, ThreadPool& pool, RateLimiter& limiter, Router& router)
    : port(port), pool(pool), limiter(limiter), router(router) {}

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
        pool.enqueue([this, clientFd, ip] { handleConnection(clientFd, ip); });
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
    std::vector<char> buf(8192);
    const ssize_t n = recv(clientFd, buf.data(), buf.size() - 1, 0);
    if (n <= 0) {
        close(clientFd);
        return;
    }
    buf[static_cast<std::size_t>(n)] = '\0';

    const HttpRequest req = HttpRequest::parse(std::string(buf.data(), static_cast<std::size_t>(n)));
    std::string response;
    if (req.method.empty() || req.path.empty()) {
        response = HttpResponse::badRequest();
    } else {
        response = router.dispatch(req);
    }

    send(clientFd, response.data(), response.size(), 0);
    close(clientFd);
}
