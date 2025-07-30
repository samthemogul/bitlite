#include "network.hpp"
#include <iostream>
#include <cstring>
#include <optional>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>


enum MessageType {
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8,
    PORT = 9
};

struct Message {
    uint32_t length;
    uint8_t id;
    std::vector<uint8_t> payload;
};

bool recv_n(int sockfd, uint8_t* buffer, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t bytes = recv(sockfd, buffer + total, n - total, 0);
        if (bytes <= 0) return false;
        total += bytes;
    }
    return true;
}

std::optional<Message> read_message(int sockfd) {
    uint8_t length_buf[4];
    if (!recv_n(sockfd, length_buf, 4)) {
        std::cerr << "Failed to read length\n";
        return std::nullopt;
    }
    uint32_t length = ntohl(*reinterpret_cast<uint32_t*>(length_buf));
    if (length == 0) {
        // Keep-alive message
        return Message{0, 255, {}};
    }
    std::cerr << "Message length: " << length << "\n";

    uint8_t id;
    if (!recv_n(sockfd, &id, 1)) {
        std::cerr << "Failed to read message ID\n";
        return std::nullopt;
    }

    std::vector<uint8_t> payload(length - 1);
    if (!recv_n(sockfd, payload.data(), payload.size())) {
        std::cerr << "Failed to read payload\n";
        return std::nullopt;
    }

    return Message{length, id, std::move(payload)};
}
