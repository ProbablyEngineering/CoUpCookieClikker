#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class CookieClickerClient {
private:
    SOCKET client_socket;

public:
    CookieClickerClient(const char* server_ip, const char* port) {
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }

        struct addrinfo hints = {}, * result;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if (getaddrinfo(server_ip, port, &hints, &result) != 0) {
            WSACleanup();
            throw std::runtime_error("getaddrinfo failed");
        }

        client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (client_socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            WSACleanup();
            throw std::runtime_error("socket creation failed");
        }

        std::cout << "Connecting to server...\n";
        if (connect(client_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            std::cerr << "Connection failed.\n";
            closesocket(client_socket);
            WSACleanup();
            return;
        }
        else {
            std::cout << "Connected to server!\n";
        }

        freeaddrinfo(result);
    }

    ~CookieClickerClient() {
        closesocket(client_socket);
        WSACleanup();
    }

    void play() {
        char buffer[1024];
        std::string input;

        while (true) {
            std::cout << "Enter 'C' to click or 'q' to quit: ";
            std::getline(std::cin, input);

            if (send(client_socket, input.c_str(), input.size(), 0) == SOCKET_ERROR) {
                std::cerr << "Send failed\n";
                break;
            }

            if (input == "q") break;

            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) break;

            buffer[bytes_received] = '\0';
            std::cout << buffer;
        }
    }
};

int main() {
    try {
        CookieClickerClient client("192.168.0.152", "12345");
        client.play();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
