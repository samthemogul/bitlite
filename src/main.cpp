#include <wifi_connect.hpp>
#include <iostream>
#include <string>

int main()
{
    try
    {
        auto [ip, port] = get_phone_ip_and_port("wlo1");
        std::cout << "Phone IP: " << ip << ", Port: " << port << std::endl;

        // Output the IP and port for server.js to use
        std::cout << "Ready to communicate with WebSocket server." << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}