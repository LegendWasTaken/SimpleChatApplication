#pragma once

#include <cstddef>
#include <vector>
#include <cstring>
#include <ctime>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <unordered_map>

namespace ca {
    class message {
    public:
        enum class sender {
            self,
            other,
            unknown
        };

        message() = default;

        message(std::uint64_t sent, std::string content) : _sender(message::sender::other), _sent(sent),
                                                           _content(std::move(content)) { _calc_hash(); }

        explicit message(std::string content) : _seen(false), _sender(message::sender::self),
                                                _sent(duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()),
                                                _content(std::move(content)) { _calc_hash(); }

        /// Serializes the message data into a vector of bytes
        /// \return Serialized data as a byte vector
        [[nodiscard]] std::vector<std::byte> as_stream() const noexcept {
            auto stream = std::vector<std::byte>(
                    sizeof(std::uint64_t) +
                    sizeof(size_t) +
                    _content.size() +
                    1);

            const auto size = _content.size();

            auto data = stream.data();
            stream[0] = std::byte(0); // The 0 means that this is a new message packet
            data++;

            std::memcpy(data, &_sent, sizeof(std::uint64_t));
            data += sizeof(std::uint64_t);

            std::memcpy(data, &size, sizeof(size_t));
            data += sizeof(size_t);

            std::memcpy(data, _content.c_str(), _content.size());

            return stream;
        }

        void set_seen() noexcept { _seen = true; }

        [[nodiscard]] bool seen() const noexcept { return _seen; }

        [[nodiscard]] ca::message::sender sent_by() const noexcept { return _sender; }

        /// The time that the message was originally created / sent
        /// \return The time in seconds since epoch
        [[nodiscard]] std::uint64_t time_sent() const noexcept { return _sent; }

        /// The message string content
        /// \return Message content
        [[nodiscard]] const std::string &content() const noexcept { return _content; }

        /// Gives you a unique identifier for the message by using it's hash
        /// \return The message unique hash
        [[nodiscard]] size_t hash() const noexcept { return _hash; }

        [[nodiscard]] bool operator==(size_t hash) const noexcept {
            return _hash == hash;
        }

        struct local_time {
            bool am;
            uint8_t second;
            uint8_t minute;
            uint8_t hour;
        };

        /// Similar to #time-sent, but instead returns a struct with a nicer human readable struct
        /// \return A struct of the local time the message was sent
        [[nodiscard]] local_time local_time_sent() const noexcept {
            auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(_sent));
            auto time = std::chrono::system_clock::to_time_t(tp);
            auto local = std::localtime(&time);
            return {.am = local->tm_hour<12,
                    .second = static_cast<std::uint8_t>(local->tm_sec),
                    .minute = static_cast<std::uint8_t>(local->tm_min),
                    .hour = static_cast<std::uint8_t>(local->tm_hour % 12)};
        }

    private:

        /// Creates a hash from the message content and timestamp, allowing for unique-identifiers
        void _calc_hash() {
            const auto combined_string = _content + std::to_string(_sent);

            // Calculating hash by hand - std::hash is nondeterministic
            _hash = size_t(37);
            for (const auto letter : combined_string)
                _hash = (_hash * 54059) ^ (letter * 76963);
            _hash %= 86969; // Using all the prime numbers possible
        }

        bool _seen = false;
        size_t _hash = ~0;
        ca::message::sender _sender = message::sender::unknown;
        std::uint64_t _sent = 0;
        std::string _content;
    };
}
