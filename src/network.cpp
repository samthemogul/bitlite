#include "network.hpp"

std::string url_encode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else
        {
            escaped << '%' << std::uppercase << std::setw(2) << int(c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string build_http_path(const std::string &info_hash, const std::string &peer_id, int port, int uploaded, int downloaded, int left, const std::string &event)
{
    std::string request = "/announce?info_hash=" + url_encode(info_hash) +
                          "&peer_id=" + url_encode(peer_id) +
                          "&port=" + std::to_string(port) +
                          "&uploaded=" + std::to_string(uploaded) +
                          "&downloaded=" + std::to_string(downloaded) +
                          "&left=" + std::to_string(left) +
                          "&event=" + event;
    return request;
}

std::string send_http_request(const std::string &host, int port, const std::string &path)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct hostent *server = gethostbyname(host.c_str());
    if (!server)
    {
        throw std::runtime_error("Host not found");
    }

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        throw std::runtime_error("Connection failed");
    }

    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Connection: close\r\n\r\n";

    send(sockfd, request.str().c_str(), request.str().length(), 0);

    char buffer[4096];
    std::ostringstream response;
    int bytesRead;
    while ((bytesRead = read(sockfd, buffer, sizeof(buffer))) > 0)
    {
        response.write(buffer, bytesRead);
    }

    close(sockfd);

    return response.str();
}

std::string send_https_request(const std::string &host, int port, const std::string &path)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        throw std::runtime_error("Unable to create SSL context");
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server = gethostbyname(host.c_str());
    if (!server)
    {
        throw std::runtime_error("Host not found");
    }

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    std::memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        throw std::runtime_error("Connection failed");
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0)
    {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        throw std::runtime_error("SSL connection failed");
    }

    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Connection: close\r\n\r\n";

    SSL_write(ssl, request.str().c_str(), request.str().length());

    char buffer[4096];
    std::ostringstream response;
    int bytesRead;
    while ((bytesRead = SSL_read(ssl, buffer, sizeof(buffer))) > 0)
    {
        response.write(buffer, bytesRead);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);

    return response.str();
}

// Splits the announce URL into its components: protocol, host, port, and path
std::vector<std::string> split_announce_url(const std::string &announce_url)
{
    std::regex url_regex(R"((\w+)://([^:/]+)(:(\d+))?(.*))");
    std::smatch match;

    std::string protocol, host, port = "80", path = "/announce"; // defaults

    if (std::regex_match(announce_url, match, url_regex))
    {
        protocol = match[1]; // http or udp
        host = match[2];     // tracker.opentrackr.org
        if (match[4].matched)
        {
            port = match[4]; // 1337
        }
        else if (protocol == "https")
        {
            port = "443";
        }
        if (match[5].matched && !match[5].str().empty())
        {
            path = match[5];
        }
    }

    return {protocol, host, port, path};
}

std::string generate_peer_id()
{
    const std::string client_prefix = "-EM0001-"; // EM = your client, 0001 = version
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, charset.size() - 1);

    std::string random_part;
    for (int i = 0; i < 12; ++i)
    {
        random_part += charset[dist(rng)];
    }

    return client_prefix + random_part; // total 20 bytes
}

int connect_to_peer_http(const std::string &ip, int port)
{
    struct addrinfo hints{}, *res, *p;
    int sockfd = -1;
    std::string port_str = std::to_string(port);

    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    int status = getaddrinfo(ip.c_str(), port_str.c_str(), &hints, &res);
    if (status != 0)
    {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return -1;
    }

    for (p = res; p != nullptr; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break; // Successfully connected
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res); // Clean up

    return sockfd; // -1 if all connections failed
}

void send_handshake(int sockfd, const std::string &info_hash, const std::string &peer_id)
{
    std::array<uint8_t, 68> handshake;

    const std::string pstr = "BitTorrent protocol";
    handshake[0] = static_cast<uint8_t>(pstr.size());

    std::memcpy(&handshake[1], pstr.c_str(), pstr.size());

    // Reserved bytes (8 zeros)
    std::memset(&handshake[1 + pstr.size()], 0, 8);

    // Copy info_hash (20 bytes)
    std::memcpy(&handshake[1 + pstr.size() + 8], info_hash.data(), 20);

    // Copy peer_id (20 bytes)
    std::memcpy(&handshake[1 + pstr.size() + 8 + 20], peer_id.data(), 20);

    // Send the handshake
    ssize_t sent = send(sockfd, handshake.data(), handshake.size(), 0);
    if (sent != static_cast<ssize_t>(handshake.size()))
    {
        std::cerr << "Failed to send full handshake" << std::endl;
    }
    else
    {
        std::cout << "Handshake sent ✅" << std::endl;
    }
}

bool receive_handshake(int sockfd)
{
    uint8_t hs[68];
    size_t total_received = 0;
    while (total_received < 68)
    {
        ssize_t received = recv(sockfd, hs + total_received, 68 - total_received, 0);
        if (received <= 0)
        {
            std::cerr << "Failed to receive peer handshake\n";
            return false;
        }
        total_received += received;
    }

    uint8_t pstrlen = hs[0];
    std::string pstr(reinterpret_cast<char *>(&hs[1]), pstrlen);

    std::string expected_pstr = "BitTorrent protocol";
    if (pstr != expected_pstr)
    {
        std::cerr << "Unexpected protocol string: " << pstr << "\n";
        return false;
    }

    // compare info hash and log the content of the handshake
    std::string info_hash(reinterpret_cast<char *>(&hs[1 + pstrlen + 8]), 20);
    std::string peer_id(reinterpret_cast<char *>(&hs[1 + pstrlen + 8 + 20]), 20);
    std::cout << "Peer info hash: " << info_hash << "\n";
    std::cout << "Peer ID: " << peer_id << "\n";
    std::cout << "✅ Received peer handshake successfully.\n";
    return true;
}

bool send_interested(int sockfd)
{
    uint8_t interested_msg[] = {0x00, 0x00, 0x00, 0x01, 0x02};
    ssize_t sent = send(sockfd, interested_msg, sizeof(interested_msg), 0);
    return sent == sizeof(interested_msg);
}

bool wait_for_unchoke(int sockfd)
{
    uint8_t buffer[5];
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (received > 0)
    {
        std::cout << "Received " << received << " bytes. Message ID: " << (int)buffer[4] << "\n";
    }
    else
    {
        std::cerr << "Failed to receive valid message.\n";
    }

    fd_set readfds;
    struct timeval timeout{};
    timeout.tv_sec = 10; // wait up to 10 seconds
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    int retries = 3; // Retry up to 3 times
    while (retries-- > 0)
    {
        int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0)
        {
            perror("select error");
            return false;
        }
        else if (ready == 0)
        {
            std::cerr << "Timeout waiting for unchoke. Retrying...\n";
            continue;
        }

        uint8_t buffer[5];
        ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
        std::cout << "Received " << received << " bytes for unchoke check.\n";

        if (received == 5 && buffer[4] == 1)
        {
            std::cout << "Peer unchoked us! Ready to request pieces.\n";
            return true;
        }
        else if (received == 5)
        {
            std::cout << "Received message ID: " << (int)buffer[4] << ", not unchoke.\n";
        }
        else
        {
            std::cerr << "Failed to receive valid message.\n";
        }
    }

    std::cerr << "Failed to receive unchoke message after retries.\n";
    return false;
}

std::vector<bool> receive_bitfield(int sockfd, size_t num_pieces)
{
    uint8_t length_prefix[4];
    ssize_t len_recv = recv(sockfd, length_prefix, 4, MSG_WAITALL);
    if (len_recv != 4)
    {
        std::cerr << "Failed to receive bitfield length prefix.\n";
        return {};
    }

    uint32_t length = (length_prefix[0] << 24) |
                      (length_prefix[1] << 16) |
                      (length_prefix[2] << 8) |
                      (length_prefix[3]);

    if (length == 0)
    {
        std::cerr << "Keep-alive or no bitfield.\n";
        return {};
    }

    std::vector<uint8_t> payload(length);
    if (recv(sockfd, payload.data(), length, MSG_WAITALL) != length)
    {
        std::cerr << "Failed to receive bitfield payload.\n";
        return {};
    }

    if (payload[0] != 5)
    {
        std::cerr << "Expected bitfield (id=5), got id=" << (int)payload[0] << "\n";
        return {};
    }

    std::vector<bool> bitfield;
    for (size_t i = 1; i < payload.size(); ++i)
    {
        for (int bit = 7; bit >= 0 && bitfield.size() < (size_t)num_pieces; --bit)
        {
            bitfield.push_back((payload[i] >> bit) & 1);
        }
    }

    std::cout << "Parsed bitfield with " << bitfield.size() << " bits.\n";
    return bitfield;
}
