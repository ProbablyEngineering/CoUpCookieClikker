#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

class CookieClickerServer {
private:
    SOCKET server_socket;
    int total_cookies;
    std::mutex cookie_mutex;

    void handle_client(SOCKET client_socket) {
        char buffer[1024];
        while (true) {
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) break; // Client disconnected
            buffer[bytes_received] = '\0';

            std::string request(buffer);
            if (request == "C") {
                {
                    std::lock_guard<std::mutex> lock(cookie_mutex);
                    total_cookies++;
                }
                std::string response = "Total Cookies: " + std::to_string(total_cookies) + "\n";
                send(client_socket, response.c_str(), response.size(), 0);
            }
            else if (request == "EXIT") {
                break;
            }
        }
        closesocket(client_socket);
    }

public:
    CookieClickerServer(const char* port) : total_cookies(0) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        struct addrinfo hints = {}, * result;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(nullptr, port, &hints, &result) != 0) {
            WSACleanup();
            throw std::runtime_error("getaddrinfo failed");
        }

        server_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (server_socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            throw std::runtime_error("socket creation failed");
        }

        // Allow address reuse (important for restarting the server)
        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
            freeaddrinfo(result);
            closesocket(server_socket);
            WSACleanup();
            throw std::runtime_error("setsockopt failed");
        }

        // Bind to the specified address and port
        if (bind(server_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            freeaddrinfo(result);
            closesocket(server_socket);
            WSACleanup();
            throw std::runtime_error("bind failed");
        }


        freeaddrinfo(result);

        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(server_socket);
            WSACleanup();
            throw std::runtime_error("listen failed");
        }

        std::cout << "Server listening on port " << port << "...\n";
    }

    ~CookieClickerServer() {
        closesocket(server_socket);
        WSACleanup();
    }

    void run() {
        while (true) {
            SOCKET client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket == INVALID_SOCKET) continue;

            std::thread(&CookieClickerServer::handle_client, this, client_socket).detach();
        }
    }
};

int main() {
    try {
        CookieClickerServer server("12345");
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
