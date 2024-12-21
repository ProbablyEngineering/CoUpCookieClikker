#include <iostream>
#include <string>
#include "GameNetworkingSockets/steam/steamnetworkingsockets.h"

class CookieClickerServer {
private:
    ISteamNetworkingSockets* networking_interface;
    HSteamListenSocket listen_socket;
    HSteamNetPollGroup poll_group;

public:
    CookieClickerServer(uint16 port) {
        SteamDatagramErrMsg err_msg;
        if (!GameNetworkingSockets_Init(nullptr, err_msg)) {
            throw std::runtime_error("Failed to initialize GameNetworkingSockets: " + std::string(err_msg));
        }

        networking_interface = SteamNetworkingSockets();

        SteamNetworkingIPAddr server_addr;
        server_addr.Clear(); // Reset the structure

        // Use ParseString to bind to all network interfaces
        std::string address = "0.0.0.0:" + std::to_string(port);
        if (!server_addr.ParseString(address.c_str())) {
            throw std::runtime_error("Invalid IP address or port: " + address);
        }

        listen_socket = networking_interface->CreateListenSocketIP(server_addr, 0, nullptr);
        poll_group = networking_interface->CreatePollGroup();

        if (listen_socket == k_HSteamListenSocket_Invalid || poll_group == k_HSteamNetPollGroup_Invalid) {
            throw std::runtime_error("Failed to create listen socket or poll group");
        }

        std::cout << "Server listening on " << address << std::endl;
    }

    ~CookieClickerServer() {
        networking_interface->CloseListenSocket(listen_socket);
        networking_interface->DestroyPollGroup(poll_group);
        GameNetworkingSockets_Kill();
    }

    void run() {
        while (true) {
            networking_interface->RunCallbacks();

            SteamNetworkingMessage_t* messages[16];
            int num_messages = networking_interface->ReceiveMessagesOnPollGroup(poll_group, messages, 16);

            for (int i = 0; i < num_messages; ++i) {
                std::string message((char*)messages[i]->m_pData, messages[i]->m_cbSize);
                std::cout << "Client: " << message << std::endl;

                // Echo the message back to the client
                networking_interface->SendMessageToConnection(messages[i]->m_conn, message.c_str(), message.size(), k_nSteamNetworkingSend_Reliable, nullptr);
                messages[i]->Release();
            }
        }
    }
};

int main() {
    try {
        CookieClickerServer server(12345);
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
