#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <regex>
#include <random>
#include <bitset>

std::string url_encode(const std::string &value);

std::string build_http_path(const std::string &info_hash, const std::string &peer_id, int port, int uploaded, int downloaded, int left, const std::string &event);

std::string send_http_request(const std::string &host, int port, const std::string &path);

std::string send_https_request(const std::string &host, int port, const std::string &path);

std::vector<std::string> split_announce_url(const std::string &announce_url);

std::string generate_peer_id();

int connect_to_peer_http(const std::string& ip, int port);

void send_handshake(int sockfd, const std::string &info_hash, const std::string &peer_id);

bool receive_handshake(int sockfd);

bool send_interested(int sockfd);

bool wait_for_unchoke(int sockfd);

std::vector<bool> receive_bitfield(int sockfd, size_t num_pieces);