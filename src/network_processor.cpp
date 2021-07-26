//
// Created by howto on 26/7/2021.
//

#include "network_processor.h"

ca::network_processor::~network_processor() {
    _processing = false;
}

void ca::network_processor::set_mode(ca::client_mode mode) {
    _mode = mode;
}

ca::client_mode ca::network_processor::mode() const noexcept {
    return _mode.load();
}

void ca::network_processor::queue_message(const ca::message &message) {
    auto guard = std::lock_guard(_outgoing_mutex);
    _outgoing.push_back(message);
}

std::vector<ca::message> ca::network_processor::incoming_messages() {
    auto guard = std::lock_guard(_incoming_mutex);
    auto messages = _incoming;
    _incoming.clear();
    return messages;
}

void ca::network_processor::_tick() {
    auto inc_guard = std::lock_guard(_incoming_mutex);
    auto out_guard = std::lock_guard(_outgoing_mutex);
    auto read_guard = std::lock_guard(_read_mutex);
    auto inc_read_guard = std::lock_guard(_inc_read_mutex);
    switch (_mode) {
        case client: {
            for (const auto &message : _outgoing) {
                const auto bytes = message.as_stream();
                _connector.write(bytes.data(), bytes.size());
            }
            _outgoing.clear();

            for (const auto hash : _read_messages) {
                auto data = std::array<std::byte, 5>();
                data[0] = std::byte(1);
                std::memcpy(data.data() + 1, &hash, sizeof(size_t));
                _connector.write(data.data(), data.size());
            }
            _read_messages.clear();

            while (true) {
                auto packet_type = std::byte();
                auto read = _connector.read(&packet_type, sizeof(std::byte));
                if (read <= 0) break; // We have no data coming in anymore, stop reading

                if (packet_type == std::byte(0)) {
                    // Read new message
                    auto time_sent = std::uint64_t();
                    read = _connector.read(&time_sent, sizeof(std::uint64_t));

                    auto char_count = size_t();
                    _connector.read(&char_count, sizeof(size_t));

                    auto content = std::string();
                    content.resize(char_count);
                    read = _connector.read(content.data(), char_count);
                    if (read != char_count)
                        std::terminate();

                    _incoming.emplace_back(time_sent, std::move(content));
                } else if (packet_type == std::byte(1)) {
                    // Message read
                    auto message_hash = size_t();
                    read = _connector.read(&message_hash, sizeof(size_t));
                    _inc_read_messages.push_back(message_hash);
                }


            }
        }
            break;
        case server: {
            for (const auto &message : _outgoing) {
                const auto bytes = message.as_stream();
                _socket.write(bytes.data(), bytes.size());
            }
            _outgoing.clear();

            for (const auto hash : _read_messages) {
                auto data = std::array<std::byte, 5>();
                data[0] = std::byte(1);
                std::memcpy(data.data() + 1, &hash, sizeof(size_t));
                _socket.write(data.data(), data.size());
            }
            _read_messages.clear();

            while (true) {
                auto packet_type = std::byte();
                auto read = _socket.read(&packet_type, sizeof(std::byte));
                if (read <= 0) break; // We have no data coming in anymore, stop reading

                if (packet_type == std::byte(0)) {
                    // Read new message
                    auto time_sent = std::uint64_t();
                    read = _socket.read(&time_sent, sizeof(std::uint64_t));

                    auto char_count = size_t();
                    _socket.read(&char_count, sizeof(size_t));

                    auto content = std::string();
                    content.resize(char_count);
                    read = _socket.read(content.data(), char_count);
                    if (read != char_count)
                        std::terminate();

                    _incoming.emplace_back(time_sent, std::move(content));
                } else if (packet_type == std::byte(1)) {
                    // Message read
                    auto message_hash = size_t();
                    read = _socket.read(&message_hash, sizeof(size_t));
                    _inc_read_messages.push_back(message_hash);
                }
            }
        }
            break;
        case unknown:
            break;
    }
}

void ca::network_processor::start() {
    _connector.set_non_blocking(true);
    _socket.set_non_blocking(true);
    _acceptor.set_non_blocking(true);

    _processing_thread = std::make_unique<std::thread>(std::thread([this]() {
        while (_processing) {
            using namespace std::chrono_literals;
//             Only poll for messages around 10 times a second
            std::this_thread::sleep_for(100ms);
            _tick();
        }
    }));
}

bool ca::network_processor::connected() const noexcept {
    return _connected;
}

void ca::network_processor::connect(const std::string &address, std::uint16_t port) {
    _connector = sockpp::tcp_connector();

    try {
        _connector.connect({address, port});
    } catch (...)
    {
        std::terminate();
    }

    _connected = true;
    start();
}

std::uint16_t ca::network_processor::create_server() {
    _connected = true;
    auto port = 50000;

    _acceptor = sockpp::tcp_acceptor();

    while (!_acceptor.open(port++));
    port--;

    _waiting_on_connection = true;

    _server_port = port;

    return port;
}

void ca::network_processor::wait_on_connection() {
    auto peer = sockpp::inet_address();
    auto socket = _acceptor.accept(&peer);

    _socket = std::move(socket);
    _waiting_on_connection = false;
    start();
}

bool ca::network_processor::waiting_on_connection() const noexcept {
    return _waiting_on_connection;
}

std::uint16_t ca::network_processor::server_port() const noexcept {
    return _server_port;
}

std::vector<size_t> ca::network_processor::read_messages() {
    auto guard = std::lock_guard(_inc_read_mutex);
    auto read = _inc_read_messages;
    return _inc_read_messages.clear(), read;
}

void ca::network_processor::seen(size_t message_hash) {
    auto guard = std::lock_guard(_read_mutex);
    _read_messages.push_back(message_hash);
}
