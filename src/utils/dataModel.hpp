#include <string>
#include <array>
#include <utility>

const int ENTRY_SERVER_PORT = 9000;
const int CLIENT_COUNT = 16;

// std::pair<std::string_view, std::string_view> split_message(const std::string_view &s, const std::string &delimiter)
// {
//     size_t pos = s.find(delimiter);
//     return {s.substr(0, pos), s.substr(pos + delimiter.length())};
// }

std::tuple<std::string_view, std::string_view, std::string_view> split_message(const std::string_view &s)
{
    size_t pos1 = s.find(";;");
    size_t pos2 = s.find(";;", pos1 + 2);
    return {s.substr(0, pos1), s.substr(pos1 + 2, pos2 - pos1 - 2), s.substr(pos2 + 2)};
}

class ISerializable
{
public:
    virtual ~ISerializable() = default;
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string &s) = 0;
};

enum class OpCode
{
    DATA = 10,
    // TEXT = 1,
    // BINARY = 2,
    // PING = 9,
    // PONG = 10,
    // CLOSE = 8
};

// enum class DataType
// {
//     INT = 10,
//     FLOAT = 20,
//     STRING = 30,
//     VECTOR_INT = 41,
//     VECTOR_FLOAT = 42,
//     MATRIX = 50,
// };

class WebSocketData
{
public:
    OpCode op;
    std::string data;
    WebSocketData() = default;
    WebSocketData(OpCode op, const ISerializable &data)
    {
        this->op = op;
        this->data = data.serialize();
    }
    WebSocketData(OpCode op, const std::string &s)
    {
        this->op = op;
        this->data = data;
    }

    static ISerializable *deserialize(OpCode op, const std::string &s)
    {
        const auto type = s.substr(0, 10);
        const auto data = s.substr(10);
        // if (type == "VECTOR_INT:")
        // {
        //     auto result = new Vector<int, 3>();
        //     result->deserialize(data);
        //     return result;
        // }
        // if (type == "INT:")
        // {
        //     auto result = new Int();
        //     result->deserialize(data);
        //     return result;
        // }
        // else if (type == "FLOAT:")
        // {
        //     auto result = new Float();
        //     result->deserialize(data);
        //     return result;
        // }
        // else if (type == "STRING:")
        // {
        //     auto result = new String();
        //     result->deserialize(data);
        //     return result;
        // }
        // else if (type == "VECTOR_FLOAT:")
        // {
        //     auto result = new Vector<float, 3>();
        //     result->deserialize(data);
        //     return result;
        // }
        // else if (type == "MATRIX:")
        // {
        //     auto result = new Matrix();
        //     result->deserialize(data);
        //     return result;
        // }
        return nullptr;
    }
};

static int DATA_SIZE = 10;

// template <class T, size_t N>
// class Vector : public ISerializable
// {
// public:
//     T data[N];
//     std::string get_type() const
//     {
//         return "VECTOR_INT:"; // only int for now
//     }

//     std::string serialize() const override
//     {
//         auto result = get_type() + std::string(reinterpret_cast<const char *>(data), sizeof(data));
//         return std::move(result);
//     }

//     void deserialize(const std::string &s) override
//     {
//         std::memcpy(data, s.data(), sizeof(data));
//     }
// };
