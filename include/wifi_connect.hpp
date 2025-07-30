#include <iostream>
#include <string>
#include <vector>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>
#include <cstring>
#include <unistd.h>  // For sleep
#include <algorithm> // For std::fill
#include <utility> // For std::pair

struct WifiNetwork
{
    std::string ssid;
    std::string bssid;
    int signal_strength; // in dBm
    int frequency;       // in MHz
};

// Function to perform Wi-Fi scan and return available networks
std::vector<WifiNetwork> perform_wifi_scan(const std::string &interface_name);

// Function to connect to a Wi-Fi network
void connect_to_wifi(const WifiNetwork &network, const std::string &passphrase, const std::string &interface_name);

// Function to get the phone's IP address and port
std::pair<std::string, int> get_phone_ip_and_port(const std::string &interface_name);

