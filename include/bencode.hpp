#ifndef BENCODE_HPP
#define BENCODE_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace bencode
{

    // Define Bencode Value Variant
    struct Bencode; // Forward declaration

    using BencodeValue = std::variant<
        int64_t,
        std::string,
        std::vector<std::shared_ptr<Bencode>>, // now holds pointers
        std::map<std::string, std::shared_ptr<Bencode>>>;

    struct Bencode
    {
        BencodeValue value;
    };

    // --- Decode Functions ---

    // Decode bencoded string into the appropraite value
    BencodeValue decode(const std::string &input);

    // Decode using iterator (for recursive parsing)
    BencodeValue decode(std::string::const_iterator &it, const std::string::const_iterator &end);

    // Encode BencodeValue into a bencoded string
    std::string encode(const BencodeValue &value);

    // --- Utility Functions ---

    // Pretty-print the value (for debugging)
    void print(const BencodeValue &value, int indent = 0);

    // Type-check helpers
    bool is_int(const BencodeValue &val);
    bool is_string(const BencodeValue &val);
    bool is_list(const BencodeValue &val);
    bool is_dict(const BencodeValue &val);

    // Getters (throw std::bad_variant_access if wrong type)
    int64_t as_int(const BencodeValue &val);
    const std::string &as_string(const BencodeValue &val);
    const std::vector<std::shared_ptr<Bencode>>&as_list(const BencodeValue &val);
    const std::map<std::string, std::shared_ptr<Bencode>> &as_dict(const BencodeValue &val);
    
}

#endif // BENCODE_HPP
