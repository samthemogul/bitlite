#include "bencode.hpp"

namespace bencode
{

    // ----------------- Type Checkers -----------------
    bool is_int(const BencodeValue &val)
    {
        return std::holds_alternative<int64_t>(val);
    }

    bool is_string(const BencodeValue &val)
    {
        return std::holds_alternative<std::string>(val);
    }

    bool is_list(const BencodeValue &val)
    {
        return std::holds_alternative<std::vector<std::shared_ptr<Bencode>>>(val);
    }

    bool is_dict(const BencodeValue &val)
    {
        return std::holds_alternative<std::map<std::string, std::shared_ptr<Bencode>>>(val);
    }

    // ----------------- Getters -----------------
    int64_t as_int(const BencodeValue &val)
    {
        return std::get<int64_t>(val);
    }

    const std::string &as_string(const BencodeValue &val)
    {
        return std::get<std::string>(val);
    }

    const std::vector<std::shared_ptr<Bencode>> &as_list(const BencodeValue &val)
    {
        return std::get<std::vector<std::shared_ptr<Bencode>>>(val);
    }

    const std::map<std::string, std::shared_ptr<Bencode>> &as_dict(const BencodeValue &val)
    {
        return std::get<std::map<std::string, std::shared_ptr<Bencode>>>(val);
    }

    std::shared_ptr<Bencode> make_int(int64_t val)
    {
        auto b = std::make_shared<Bencode>();
        b->value = val;
        return b;
    }

    // ----------------- Encoder -----------------
    std::string encode(const BencodeValue &value)
    {
        std::ostringstream oss;

        if (is_int(value))
        {
            oss << 'i' << as_int(value) << 'e';
        }
        else if (is_string(value))
        {
            const auto &str = as_string(value);
            oss << str.length() << ':' << str;
        }
        else if (is_list(value))
        {
            oss << 'l';
            for (const auto &item : as_list(value))
            {
                oss << encode(item->value);
            }
            oss << 'e';
        }
        else if (is_dict(value))
        {
            oss << 'd';
            for (const auto &[key, item] : as_dict(value))
            {
                oss << key.length() << ':' << key;
                oss << encode(item->value);
            }
            oss << 'e';
        }

        return oss.str();
    }

    // ----------------- Decoder -----------------
    BencodeValue decode(const std::string &input)
    {
        auto it = input.begin();
        return decode(it, input.end());
    }

    BencodeValue decode(std::string::const_iterator &it, const std::string::const_iterator &end)
    {
        if (it == end)
            throw std::runtime_error("Unexpected end of input");

        char c = *it;
        if (c == 'i')
        {
            // Integer
            ++it;
            std::string num;
            while (it != end && *it != 'e')
            {
                num += *it++;
            }
            if (it == end)
                throw std::runtime_error("Unterminated integer");
            ++it; // skip 'e'
            int64_t val = std::stoll(num);
            return BencodeValue{val};
        }
        else if (isdigit(c))
        {
            // String
            std::string len_str;
            while (it != end && *it != ':')
            {
                len_str += *it++;
            }
            if (it == end || *it != ':')
                throw std::runtime_error("Invalid string format");
            ++it; // skip ':'
            size_t len = std::stoul(len_str);
            if (std::distance(it, end) < static_cast<long>(len))
                throw std::runtime_error("Not enough characters for string");

            std::string str(it, it + len);
            it += len;
            return str;
        }
        else if (c == 'l')
        {
            // List
            ++it;
            std::vector<std::shared_ptr<Bencode>> list;
            while (it != end && *it != 'e')
            {
                auto val = std::make_shared<Bencode>();
                val->value = decode(it, end); // decode returns BencodeValue
                list.push_back(val);
            }
            if (it == end)
                throw std::runtime_error("Unterminated list");
            ++it; // skip 'e'
            return list;
        }
        else if (c == 'd')
        {
            // Dictionary
            ++it;
            std::map<std::string, std::shared_ptr<Bencode>> dict;
            while (it != end && *it != 'e')
            {
                // Parse key
                std::string key;
                if (!std::isdigit(*it))
                    throw std::runtime_error("Invalid dictionary key");

                key = std::get<std::string>(decode(it, end)); // decode key

                // Parse value
                auto val = std::make_shared<Bencode>();
                val->value = decode(it, end); // decode value
                dict[key] = val;
            }
            if (it == end)
                throw std::runtime_error("Unterminated dict");
            ++it; // skip 'e'
            return dict;
        }

        throw std::runtime_error(std::string("Unknown type: ") + c);
    }

    // ----------------- Printer -----------------
    void print(const BencodeValue &value, int indent)
    {
        std::string pad(indent, ' ');

        if (is_int(value))
        {
            std::cout << pad << "int: " << as_int(value) << '\n';
        }
        else if (is_string(value))
        {
            std::cout << pad << "string: " << as_string(value) << '\n';
        }
        else if (is_list(value))
        {
            std::cout << pad << "list:\n";
            for (const auto &item : as_list(value))
            {
                print(item->value, indent + 2);
            }
        }
        else if (is_dict(value))
        {
            std::cout << pad << "dict:\n";
            for (const auto &[key, item] : as_dict(value))
            {
                std::cout << pad << "  key: " << key << '\n';
                print(item->value, indent + 4);
            }
        }
    }

} // namespace bencode
