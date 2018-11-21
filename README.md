# netpp - inter-process communication
[![Build Status](https://travis-ci.org/volcoma/netpp.svg?branch=master)](https://travis-ci.org/volcoma/netpp)
[![Build status](https://ci.appveyor.com/api/projects/status/euy2uypo4uml2qqi?svg=true)](https://ci.appveyor.com/project/volcoma/netpp)

C++14 library providing easy interface for inter-process communication .
It expects an installed OpenSSL

```C++
namespace net
{
// Specialize this for your type of messages (T)
// and your type of serializer/deserializer.
// Here is an example for any T and std::stringstream
template <typename T>
struct serializer<T, std::stringstream, std::stringstream>
{
    static byte_buffer to_buffer(const T& msg)
    {
        std::stringstream serializer;
        serializer << msg;
        auto str = serializer.str();
        return {std::begin(str), std::end(str)};
    }
    static T from_buffer(byte_buffer&& buffer)
    {
        std::stringstream deserializer({std::begin(buffer), std::end(buffer)});
        T msg{};
        deserializer >> msg;
        return msg;
    }
};

// specialized for std::string to respect the white spaces
template <>
struct serializer<std::string, std::stringstream, std::stringstream>
{
    static byte_buffer to_buffer(const std::string& msg)
    {
        return {std::begin(msg), std::end(msg)};
    }
    static std::string from_buffer(byte_buffer&& buffer)
    {
        return {std::begin(buffer), std::end(buffer)};
    }
};
// Get a messenger for your type of message and the serialization streams used
// to convert your (T) to byte_buffer.
// Here we provide an utility getter for all types with std::stringstream serializer/deserializer
template <typename T>
auto get_network()
{
    return get_messenger<T, std::stringstream, std::stringstream>();
}
}

int main()
{
    // Init inter-thread communication library(For more info on this visit https://github.com/volcoma/itc)
    // and init net services.
    itc::init();
    net::set_logger([](const std::string& msg) { std::cout << msg << std::endl; });
    net::init_services(std::thread::hardware_concurrency());

    std::string address = "::1"; // localhost ipv6
    uint16_t port = 1111;
    // For example.
    using msg_type = std::string;

    auto net = net::get_network<msg_type>();
    // Create a connector.
    auto server = net::create_tcp_server(port);
    // Set a msg builder. It is responsible for your data format.
    // For example : your message format may consist of header + payload
    // We provide a simple one.
    server->create_builder = net::msg_builder::get_creator<net::single_buffer_builder>();

    // Create a connector.
    auto client = net::create_tcp_client(address, port);
    // Set a msg builder. It is responsible for your data format.
    // For example : your message format may consist of header + payload
    // We provide a simple one.
    client->create_builder = net::msg_builder::get_creator<net::single_buffer_builder>();

    // Add the connector. You can add as many connectors as you want to a
    // single messenger.
    net->add_connector(server,
    [](net::connection::id_t id)
    {
        net::log() << "Server Connected with client " << id;

        auto net = net::get_network<msg_type>();
        net->send_msg(id, "echo");
    },
    [](net::connection::id_t id, const net::error_code& ec)
    {
        net::log() << "Server Disconnected from client " << id << ". Reason : " << ec.message();
    },
    [](net::connection::id_t id, msg_type msg)
    {
        net::log() << "Server Msg received from client " << id << " : " << msg;

        auto net = net::get_network<msg_type>();
        net->send_msg(id, std::move(msg));
    });

    net->add_connector(client,
    [](net::connection::id_t id)
    {
        net::log() << "Client Connected.";
    },
    [](net::connection::id_t id, const net::error_code& ec)
    {
        net::log() << "Client Disconnected. Reason : " << ec.message();
    },
    [](net::connection::id_t id, msg_type msg)
    {
        net::log() << "Client Msg received : " << msg;

        auto net = net::get_network<msg_type>();
        net->send_msg(id, std::move(msg));
    });

    while(!itc::this_thread::notified_for_exit())
    {
        //Sleep and process tasks
        itc::this_thread::sleep_for(16ms);
    }
  
    net::deinit_messengers();
    net::deinit_services();
    itc::shutdown();
}
