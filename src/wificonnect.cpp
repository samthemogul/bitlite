#include <wifi_connect.hpp>

static int scan_callback(struct nl_msg *msg, void *arg)
{
    std::vector<WifiNetwork> *networks = static_cast<std::vector<WifiNetwork> *>(arg);
    struct genlmsghdr *gnlh = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1] = {};

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

    if (gnlh->cmd != 6 /* NL80211_CMD_NEW_BSS */ &&
        gnlh->cmd != 34 /* NL80211_CMD_DUMP_BEACONS */)
    {
        return NL_SKIP;
    }

    if (!tb[NL80211_ATTR_BSS])
    {
        return NL_SKIP;
    }

    static struct nla_policy bss_policy[NL80211_BSS_MAX + 1];
    std::memset(bss_policy, 0, sizeof(bss_policy));

    bss_policy[NL80211_BSS_BSSID].type = NLA_UNSPEC;
    bss_policy[NL80211_BSS_FREQUENCY].type = NLA_U32;
    bss_policy[NL80211_BSS_SIGNAL_MBM].type = NLA_U32;
    bss_policy[NL80211_BSS_INFORMATION_ELEMENTS].type = NLA_UNSPEC;

    struct nlattr *bss_tb[NL80211_BSS_MAX + 1];

    if (nla_parse_nested(bss_tb, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy) < 0)
    {
        std::cerr << "scan_callback: Failed to parse BSS attributes (nla_parse_nested failed)." << std::endl;
        return NL_SKIP;
    }

    WifiNetwork network;
    bool ssid_found = false;

    if (bss_tb[NL80211_BSS_INFORMATION_ELEMENTS])
    {
        struct nlattr *ie_attr = bss_tb[NL80211_BSS_INFORMATION_ELEMENTS];
        const unsigned char *ies = (const unsigned char *)nla_data(ie_attr);
        size_t ies_len = nla_len(ie_attr);

        for (size_t i = 0; i < ies_len;)
        {
            if (i + 1 >= ies_len)
            {
                break;
            }
            unsigned char ie_type = ies[i];
            unsigned char ie_len = ies[i + 1];

            if (i + 2 + ie_len > ies_len)
            {
                break;
            }

            if (ie_type == 0x00)
            { // SSID IE
                if (ie_len > 0)
                {
                    network.ssid = std::string(reinterpret_cast<const char *>(&ies[i + 2]), ie_len);
                }
                else
                {
                    network.ssid = "<hidden/empty SSID>";
                }
                ssid_found = true;
                break;
            }
            i += 2 + ie_len;
        }

        if (!ssid_found)
        {
            network.ssid = "<hidden or unparseable>";
        }
    }
    else
    {
        network.ssid = "<hidden>";
    }

    if (bss_tb[NL80211_BSS_BSSID])
    {
        char mac_str[18];
        unsigned char *mac_addr = static_cast<unsigned char *>(nla_data(bss_tb[NL80211_BSS_BSSID]));
        snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        network.bssid = mac_str;
    }
    else
    {
        network.bssid = "<unknown>";
    }

    if (bss_tb[NL80211_BSS_FREQUENCY])
    {
        network.frequency = nla_get_u32(bss_tb[NL80211_BSS_FREQUENCY]);
    }
    else
    {
        network.frequency = 0;
    }

    if (bss_tb[NL80211_BSS_SIGNAL_MBM])
    {
        network.signal_strength = (int)nla_get_u32(bss_tb[NL80211_BSS_SIGNAL_MBM]) / 100;
    }
    else
    {
        network.signal_strength = -100;
    }

    networks->push_back(network);
    return NL_OK;
}

static int dummy_callback(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

// Function to perform a Wi-Fi scan and return results
int perform_scan(struct nl_sock *sk_cmd, struct nl_sock *sk_evt, int nl80211_id, int if_index, struct nl_cb *cb, struct nl_cb *cb_evt, std::vector<WifiNetwork> &networks)
{
    networks.clear(); // Clear previous scan results

    struct nl_msg *msg_scan = nullptr; // Initialize to nullptr
    msg_scan = nlmsg_alloc();
    if (!msg_scan)
    {
        std::cerr << "Failed to allocate netlink message for scan." << std::endl;
        return -ENOMEM;
    }

    genlmsg_put(msg_scan, 0, 0, nl80211_id, 0, NLM_F_REQUEST, NL80211_CMD_TRIGGER_SCAN, 0);
    nla_put_u32(msg_scan, NL80211_ATTR_IFINDEX, if_index);

    std::cout << "Initiating Wi-Fi scan..." << std::endl;
    int ret = nl_send_auto_complete(sk_cmd, msg_scan);
    if (ret < 0)
    {
        std::cerr << "Failed to send scan trigger: " << nl_geterror(-ret) << std::endl;
        nlmsg_free(msg_scan);
        return ret;
    }
    nlmsg_free(msg_scan); // Free immediately after sending

    // Receive the ACK for the TRIGGER_SCAN on the command socket
    ret = nl_recvmsgs(sk_cmd, cb);
    if (ret < 0)
    {
        std::cerr << "Failed to receive TRIGGER_SCAN ACK: " << nl_geterror(-ret) << std::endl;
    }

    sleep(5); // Give the scan time to complete

    // Get Scan Results (dump existing BSS entries)
    struct nl_msg *msg_get_scan = nullptr; // Initialize to nullptr
    msg_get_scan = nlmsg_alloc();
    if (!msg_get_scan)
    {
        std::cerr << "Failed to allocate netlink message for get scan results." << std::endl;
        return -ENOMEM;
    }

    genlmsg_put(msg_get_scan, 0, 0, nl80211_id, 0, NLM_F_DUMP, NL80211_CMD_GET_SCAN, 0);
    nla_put_u32(msg_get_scan, NL80211_ATTR_IFINDEX, if_index);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, scan_callback, &networks); // Set scan_callback for parsing

    std::cout << "Retrieving scan results..." << std::endl;
    ret = nl_send_auto_complete(sk_cmd, msg_get_scan);
    if (ret < 0)
    {
        std::cerr << "Failed to send get scan results request (dump): " << nl_geterror(-ret) << std::endl;
        nlmsg_free(msg_get_scan);
        return ret;
    }
    nlmsg_free(msg_get_scan); // Free immediately after sending

    ret = nl_recvmsgs(sk_cmd, cb);
    if (ret < 0)
    {
        std::cerr << "Failed to receive scan results (dump): " << nl_geterror(-ret) << std::endl;
        return ret;
    }
    return 0; // Success
}

// Function to perform Wi-Fi scan and return available networks
std::vector<WifiNetwork> perform_wifi_scan(const std::string &interface_name)
{
    std::vector<WifiNetwork> available_networks;
    struct nl_sock *sk_cmd = nl_socket_alloc();
    struct nl_sock *sk_evt = nl_socket_alloc();
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    struct nl_cb *cb_evt = nl_cb_alloc(NL_CB_DEFAULT);

    if (!sk_cmd || !sk_evt || !cb || !cb_evt)
    {
        throw std::runtime_error("Failed to allocate netlink resources.");
    }

    struct ResourceGuard
    {
        struct nl_sock *&sk_cmd;
        struct nl_sock *&sk_evt;
        struct nl_cb *&cb;
        struct nl_cb *&cb_evt;

        ~ResourceGuard()
        {
            if (cb)
                nl_cb_put(cb);
            if (cb_evt)
                nl_cb_put(cb_evt);
            if (sk_cmd)
                nl_socket_free(sk_cmd);
            if (sk_evt)
                nl_socket_free(sk_evt);
        }
    } resourceGuard{sk_cmd, sk_evt, cb, cb_evt};

    if (genl_connect(sk_cmd) || genl_connect(sk_evt))
    {
        throw std::runtime_error("Failed to connect to generic netlink.");
    }

    int nl80211_id = genl_ctrl_resolve(sk_cmd, "nl80211");
    if (nl80211_id < 0)
    {
        throw std::runtime_error("nl80211 not found.");
    }

    int if_index = if_nametoindex(interface_name.c_str());
    if (if_index == 0)
    {
        throw std::runtime_error("Could not find interface " + interface_name);
    }

    if (perform_scan(sk_cmd, sk_evt, nl80211_id, if_index, cb, cb_evt, available_networks) < 0)
    {
        throw std::runtime_error("Failed to perform Wi-Fi scan.");
    }

    return available_networks;
}

// Function to connect to a Wi-Fi network
void connect_to_wifi(const WifiNetwork &network, const std::string &passphrase, const std::string &interface_name)
{
    struct nl_sock *sk_cmd = nl_socket_alloc();
    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);

    if (!sk_cmd || !cb)
    {
        throw std::runtime_error("Failed to allocate netlink resources.");
    }

    struct ResourceGuard
    {
        struct nl_sock *&sk_cmd;
        struct nl_cb *&cb;

        ~ResourceGuard()
        {
            if (cb)
                nl_cb_put(cb);
            if (sk_cmd)
                nl_socket_free(sk_cmd);
        }
    } resourceGuard{sk_cmd, cb};

    if (genl_connect(sk_cmd))
    {
        throw std::runtime_error("Failed to connect to generic netlink.");
    }

    int nl80211_id = genl_ctrl_resolve(sk_cmd, "nl80211");
    if (nl80211_id < 0)
    {
        throw std::runtime_error("nl80211 not found.");
    }

    int if_index = if_nametoindex(interface_name.c_str());
    if (if_index == 0)
    {
        throw std::runtime_error("Could not find interface " + interface_name);
    }

    struct nl_msg *msg_connect = nlmsg_alloc();
    if (!msg_connect)
    {
        throw std::runtime_error("Failed to allocate netlink message for connect.");
    }

    genlmsg_put(msg_connect, 0, 0, nl80211_id, 0, NLM_F_REQUEST, NL80211_CMD_CONNECT, 0);
    nla_put_u32(msg_connect, NL80211_ATTR_IFINDEX, if_index);
    nla_put(msg_connect, NL80211_ATTR_SSID, network.ssid.length(), network.ssid.c_str());

    unsigned char bssid_bytes[6];
    if (sscanf(network.bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &bssid_bytes[0], &bssid_bytes[1], &bssid_bytes[2],
               &bssid_bytes[3], &bssid_bytes[4], &bssid_bytes[5]) == 6)
    {
        nla_put(msg_connect, NL80211_ATTR_BSSID, 6, bssid_bytes);
    }

    if (!passphrase.empty())
    {
        // Add passphrase handling here if needed
    }

    int ret_connect = nl_send_auto_complete(sk_cmd, msg_connect);
    if (ret_connect < 0)
    {
        throw std::runtime_error("Failed to send connect command.");
    }

    nlmsg_free(msg_connect);
}

// Function to get the phone's IP address and port
std::pair<std::string, int> get_phone_ip_and_port(const std::string &interface_name)
{
    std::vector<WifiNetwork> networks = perform_wifi_scan(interface_name);

    if (networks.empty())
    {
        throw std::runtime_error("No Wi-Fi networks found.");
    }

    // Number the networks displaying the available networks and let the user select one
    std::cout << "Select from available Wi-Fi Networks:" << std::endl;
    for (size_t i = 0; i < networks.size(); ++i)
    {
        std::cout << i + 1 << ". " << networks[i].ssid
                  << " (BSSID: " << networks[i].bssid
                  << ", Signal: " << networks[i].signal_strength << " dBm)"
                  << ", Freq: " << networks[i].frequency << " MHz" << std::endl;
    }
    int selected_index;
    std::cout << "Enter the number of the network you want to connect to: ";
    std::cin >> selected_index; 
    if (selected_index < 1 || selected_index > networks.size())
    {
        throw std::runtime_error("Invalid selection.");
    }
    WifiNetwork selected_network = networks[selected_index - 1];

    connect_to_wifi(selected_network, "", interface_name);

    std::string cmd_get_gateway = "ip route show dev " + interface_name + " | grep \"default via\"";
    FILE *pipe = popen(cmd_get_gateway.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("Failed to run command to get default gateway.");
    }

    char buffer[256];
    std::string gateway_ip;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::string line = buffer;
        size_t via_pos = line.find("via ");
        size_t proto_pos = line.find(" proto ");

        if (via_pos != std::string::npos && proto_pos != std::string::npos && via_pos < proto_pos)
        {
            gateway_ip = line.substr(via_pos + 4, proto_pos - (via_pos + 4));
        }
    }
    pclose(pipe);

    if (gateway_ip.empty())
    {
        gateway_ip = "0.0.0.1"; // Default fallback IP
    }

    int port = 6000; // Default port
    return {gateway_ip, port};
}

