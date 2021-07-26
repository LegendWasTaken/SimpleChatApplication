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
        network_processor();

        ~network_processor();

        /// Starts the processing thread to process incoming and outgoing packets
        void start();

        /// Set the processor mode
        /// \param mode Either server, or client. Unknown will result in undefined behaviour
        void set_mode(ca::client_mode mode);

        /// Connect the processor to the specified server (Only legal if the mode is client)
        /// \param address The server address (localhost: 127.0.0.1)
        /// \param port The server port (Typically 50000, displayed on the server information screen)
        void connect(const std::string &address, std::uint16_t port);

        /// Waits for a connection to be established from a client (Only legal if the mode is server)
        void wait_on_connection();

        /// Tells the processor to notify the other side that we've read a message
        /// \param message_hash The hash of the message that's been read
        void seen(size_t message_hash);

        /// If you're waiting for a connection as a server
        /// \return Waiting for a connection
        [[nodiscard]] bool waiting_on_connection() const noexcept;

        /// This creates a server (Only legal if the mode is server)
        /// \return The port that the server was created on
        [[nodiscard]] std::uint16_t create_server();

        /// Only legal if the mode is server
        /// \return The current port the server is bound to
        [[nodiscard]] std::uint16_t server_port() const noexcept;

        /// Is the processor connected to another one
        /// \return true if there is a connection, false if not
        [[nodiscard]] bool connected() const noexcept;

        /// The current operating mode of the server
        /// \return either client, server, unknown
        [[nodiscard]] ca::client_mode mode() const noexcept;

        /// Queue a new message to be sent to the connected processor
        /// \param message The message to be sent
        void queue_message(const ca::message& message);

        /// Get the messages that have been processed by the network processor
        /// \return The messages to process
        [[nodiscard]] std::vector<ca::message> incoming_messages();

        /// Get the messages that have been read by the other client
        /// \return The read messages
        [[nodiscard]] std::vector<size_t> read_messages();

    private:
        /// Internal function: The main processing loop that is executed on another thread
        void _tick();

        bool _connected = false;
        bool _waiting_on_connection = false;

        std::uint16_t _server_port = 0;

        // Low Priority - Benchmark using different multi threading solutions to store this
        std::atomic<ca::client_mode> _mode = ca::client_mode::unknown; // Default to client
        std::atomic<bool> _processing = true;
        std::atomic<bool> _running = false;

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

        std::thread _processing_thread;
    };
}
