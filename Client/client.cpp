#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"

class CookieClickerClient {
private:
    ISteamNetworkingSockets* networking_interface;
    HSteamNetConnection connection;
    bool connected;

public:
    CookieClickerClient(const char* server_ip, uint16 port) {
        SteamDatagramErrMsg err_msg;
        if (!GameNetworkingSockets_Init(nullptr, err_msg)) {
            throw std::runtime_error("Failed to initialize GameNetworkingSockets: " + std::string(err_msg));
        }

        networking_interface = SteamNetworkingSockets();

        SteamNetworkingIPAddr server_addr;
        server_addr.Clear();

        // Use ParseString for IP address
        std::string server_address = std::string(server_ip) + ":" + std::to_string(port);
        if (!server_addr.ParseString(server_address.c_str())) {
            throw std::runtime_error("Invalid IP address or port: " + server_address);
        }

        SteamNetworkingConfigValue_t options[] = {
            {k_ESteamNetworkingConfig_TimeoutInitial, k_ESteamNetworkingConfig_Int32, {.m_int32 = 5000}}, // Initial timeout
            {k_ESteamNetworkingConfig_TimeoutConnected, k_ESteamNetworkingConfig_Int32, {.m_int32 = 10000}} // Connected timeout
        };

        connection = networking_interface->ConnectByIPAddress(server_addr, 2, options);

        if (connection == k_HSteamNetConnection_Invalid) {
            throw std::runtime_error("Failed to create connection");
        }

        connected = true;
    }

    ~CookieClickerClient() {
        if (connected) {
            networking_interface->CloseConnection(connection, 0, "Client disconnected", true);
        }
        GameNetworkingSockets_Kill();
    }

    void play() {
        while (connected) {
            SteamNetworkingMessage_t* messages[16];
            int num_messages = networking_interface->ReceiveMessagesOnConnection(connection, messages, 16);

            for (int i = 0; i < num_messages; ++i) {
                std::string message((char*)messages[i]->m_pData, messages[i]->m_cbSize);
                std::cout << "Server: " << message << std::endl;
                messages[i]->Release();
            }

            // Input logic
            std::string input;
            std::cout << "Enter 'C' to click or 'q' to quit: ";
            std::getline(std::cin, input);

            if (input == "q") {
                connected = false;
                break;
            }

            networking_interface->SendMessageToConnection(connection, input.c_str(), input.size(), k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
};

int main() {
    try {
        CookieClickerClient client("192.168.0.152", 12345);
        client.play();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
