#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <memory>

#include <client_mode.h>
#include <message.h>

#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_connector.h>

namespace ca {
    class network_processor {
    public:
        network_processor() = default;

        ~network_processor();

        void start();

        void set_mode(ca::client_mode mode);

        void connect(const std::string &address, std::uint16_t port);

        void wait_on_connection();

        void seen(size_t message_hash);

        [[nodiscard]] bool waiting_on_connection() const noexcept;

        [[nodiscard]] std::uint16_t create_server();

        [[nodiscard]] std::uint16_t server_port() const noexcept;

        [[nodiscard]] bool connected() const noexcept;

        [[nodiscard]] ca::client_mode mode() const noexcept;

        void queue_message(const ca::message& message);

        [[nodiscard]] std::vector<ca::message> incoming_messages();

        [[nodiscard]] std::vector<size_t> read_messages();

    private:
        void _tick();

        bool _connected = false;
        bool _waiting_on_connection = false;

        std::uint16_t _server_port = 0;

        // Low Priority - Benchmark using different multi threading solutions to store this
        std::atomic<ca::client_mode> _mode = ca::client_mode::unknown; // Default to client
        std::atomic<bool> _processing = true;

        std::mutex _incoming_mutex;
        std::vector<ca::message> _incoming;

        std::mutex _outgoing_mutex;
        std::vector<ca::message> _outgoing;

        std::mutex _read_mutex;
        std::vector<size_t> _read_messages;

        std::mutex _inc_read_mutex;
        std::vector<size_t> _inc_read_messages;

        sockpp::tcp_connector  _connector;
        sockpp::tcp_acceptor _acceptor;
        sockpp::tcp_socket _socket;

        std::unique_ptr<std::thread> _processing_thread;
    };
}
