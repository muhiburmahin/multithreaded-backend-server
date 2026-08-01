#include "Server.hpp"

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

std::string guessContentType(const std::string& name) {
    const auto dot = name.rfind('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    const std::string ext = name.substr(dot);
    if (ext == ".html" || ext == ".htm") {
        return "text/html";
    }
    if (ext == ".txt") {
        return "text/plain";
    }
    if (ext == ".css") {
        return "text/css";
    }
    if (ext == ".js") {
        return "application/javascript";
    }
    if (ext == ".json") {
        return "application/json";
    }
    return "application/octet-stream";
}

// GET /files/<name> from ../public (when run from build/) or ./public
std::string servePublicFile(const std::string& name) {
    if (name.empty() || name.find("..") != std::string::npos ||
        name.find('/') != std::string::npos || name.find('\\') != std::string::npos) {
        return HttpResponse::notFound();
    }

    std::ifstream in("../public/" + name, std::ios::binary);
    if (!in) {
        in.open("public/" + name, std::ios::binary);
    }
    if (!in) {
        return HttpResponse::notFound();
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return HttpResponse::ok(ss.str(), guessContentType(name));
}

int statusFromResponse(const std::string& response) {
    // "HTTP/1.1 200 OK\r\n..."
    const auto sp1 = response.find(' ');
    if (sp1 == std::string::npos) {
        return 0;
    }
    const auto sp2 = response.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) {
        return 0;
    }
    try {
        return std::stoi(response.substr(sp1 + 1, sp2 - sp1 - 1));
    } catch (...) {
        return 0;
    }
}

}  // namespace

Server::Server(int port, ThreadPool& pool, RateLimiter& limiter, Router& router,
               Logger& logger)
    : port(port), pool(pool), limiter(limiter), router(router), logger(logger) {}

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
            if (!running) {
                break;
            }
            if (errno == EINTR) {
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
    std::cout << "Server stopped." << std::endl;
}

void Server::stop() {
    running = false;
    if (listenFd >= 0) {
        const int fd = listenFd;
        listenFd = -1;
        close(fd);
    }
}

void Server::handleConnection(int clientFd, const std::string& ip) {
    if (!limiter.allow(ip)) {
        const std::string response = HttpResponse::tooManyRequests();
        logger.log(ip, "-", "-", 429);
        send(clientFd, response.data(), response.size(), 0);
        close(clientFd);
        return;
    }

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
    } else if (req.method == "GET" && req.path.rfind("/files/", 0) == 0) {
        response = servePublicFile(req.path.substr(7));
    } else {
        response = router.dispatch(req);
    }

    const int status = statusFromResponse(response);
    logger.log(ip, req.method.empty() ? "-" : req.method,
               req.path.empty() ? "-" : req.path, status);

    send(clientFd, response.data(), response.size(), 0);
    close(clientFd);
}
