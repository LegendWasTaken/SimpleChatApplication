#pragma once

#include <string>
#include <optional>
#include <array>
#include <vector>
#include <algorithm>

#include <client_mode.h>
#include <network_processor.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_internal.h>

namespace ca {
    class display {
    public:

        display();

        /// Is the display currently running, false when the display should be closed
        /// \return If the display is currently running
        [[nodiscard]] bool running() const noexcept;

        /// The main display loop for the interfacing with the user
        /// \param processor The network processor for incoming and outgoing message handling
        void render(ca::network_processor &processor) const noexcept;

    private:

        bool _focused; /// If the window is currently selected

        GLFWwindow *_window; /// A handle to the GLFW window
    };

    namespace ui {
        struct init_ctx {
            ImGuiDockNodeFlags dock_flags;
            ImGuiWindowFlags window_flags;
            ImGuiViewport *viewport;
        };

        /// Initialize the required ImGui docks and windows
        /// \return The created ImGui context
        [[nodiscard]] inline init_ctx init() {
            auto dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            auto window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            auto viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            if (dock_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            auto ctx = init_ctx();
            ctx.dock_flags = dock_flags;
            ctx.window_flags = window_flags;
            ctx.viewport = viewport;
            return ctx;
        }

        /// Display the mode selector
        /// \return An optional of the selected (clicked) mode
        [[nodiscard]] inline std::optional<ca::client_mode> mode_selector() {
            ImGui::Begin("Select Mode", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            auto mode = std::optional<ca::client_mode>();

            if (ImGui::Button("Server"))
                mode = ca::client_mode::server;

            ImGui::SameLine();
            if (ImGui::Button("Client"))
                mode = ca::client_mode::client;

            ImGui::End();
            return mode;
        }

        struct server_address {
            std::string address;
            std::uint16_t port;
        };

        /// Display the server selector screen, this allows the user to input the server address and port
        /// \return An optional server data
        [[nodiscard]] inline std::optional<server_address> server_selector() {
            ImGui::Begin("Target Server");
            auto address = std::optional<server_address>();

            static auto address_string = std::array<char, 65>();
            ImGui::InputText("Address", address_string.data(), 64);

            static auto port = std::int32_t();
            ImGui::InputInt("Port", &port);

            if (ImGui::Button("Connect"))
                address = server_address{.address = std::string(
                        address_string.data()), .port = static_cast<std::uint16_t>(port)};

            ImGui::End();
            return address;
        }

        /// Displays server information for when the server is created and waiting for a client connection
        inline void display_server_information(std::uint16_t port) {
            ImGui::Begin("Server Information");
            ImGui::Text("Target port is %hu", port);
            ImGui::End();
        }

        /// Displays the welcome screens, where you select the mode, and connect to the server, or shows server info
        /// \param processor The network processor that is currently being used by the chat application
        /// \return true if you should display the chat screen or not
        [[nodiscard]] inline bool display_start_screen(ca::network_processor &processor) {
            // In the start, this if statement will be hit because no mode will be selected
            if (processor.mode() == ca::client_mode::unknown) {
                if (const auto selected_mode = mode_selector(); selected_mode.has_value())
                    processor.set_mode(selected_mode.value());
            }

            // If we're not connected, either wait for a connection (server), or ask for a target server (client)
            if (!processor.connected()) {
                switch (processor.mode()) {
                    case client:
                        if (const auto server = server_selector(); server.has_value())
                            processor.connect(server->address, server->port);
                        break;
                    case server:
                        processor.create_server();
                        break;
                    default:
                        break;
                }
            } else if (processor.waiting_on_connection()) {
                display_server_information(processor.server_port());

                // We wait a frame before actually waiting for the client to connect
                // This is because if we don't, the display wont rerender with the server information
                // Todo: Update the waiting to be done on the processing thread pre-connection to mitigate this
                if (processor.waiting_on_connection())
                {
                    static auto frame_delay = 0;
                    if (frame_delay++ > 1)
                        processor.wait_on_connection();
                }
            } else
                return true;
            return false;
        }

        /// Initialize the ImGui window and dockspace IDs
        /// \param ctx The ImGui context
        /// \param center  The name of the center window (currently hidden)
        inline void init_dock(
                const init_ctx &ctx,
                const std::string &center) {
            auto dockspace_id = ImGui::GetID("DockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ctx.dock_flags);

            static auto first_time = true;
            if (first_time) {
                first_time = false;

                ImGui::DockBuilderRemoveNode(dockspace_id);    // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, ctx.dock_flags | ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ctx.viewport->Size);

                ImGui::DockBuilderGetNode(dockspace_id)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

                ImGui::DockBuilderDockWindow(center.c_str(), dockspace_id);

                ImGui::DockBuilderFinish(dockspace_id);
            }
        }

        inline void root_node(init_ctx ui_ctx) {
            ImGui::Begin("DockSpace", nullptr, ui_ctx.window_flags);
            ImGui::PopStyleVar(3);

            // Setup the dock
            ui::init_dock(ui_ctx, "Chat");

            ImGui::End();
        }

        struct user_chat {
            bool send = false;
            std::string current_message;
        };

        /// Display a chat message sent from the other user
        /// \param message The message to display
        inline void display_other_chat(const ca::message &message) {
            const auto time = message.local_time_sent();
            ImGui::Text("[%02hhu:%02hhu %s] - %s", time.hour, time.minute, time.am ? "AM" : "PM",
                        message.content().c_str());
        }

        /// Display a chat message sent from yourself
        /// \param msg The message from yourself
        inline void display_your_chat(const ca::message &msg) {
            const auto time = msg.local_time_sent();
            const auto tail_message = std::format(" [%02hhu:%02hhu %s] - You", time.hour, time.minute,
                                                  time.am ? "AM" : "PM");
            auto text_width =
                    ImGui::CalcTextSize(msg.content().c_str()).x + ImGui::CalcTextSize(tail_message.c_str()).x;
            if (msg.seen())
                text_width += ImGui::CalcTextSize(" (Read)").x;
            ImGui::NewLine();
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - text_width);

            ImGui::Text("%s [%02hhu:%02hhu %s] - You%s", msg.content().c_str(), time.hour, time.minute,
                        time.am ? "AM" : "PM", msg.seen() ? " (Read)" : "");
        }

        /// Display the chat interaction between both clients
        /// \param messages a vector of the chat messages
        /// \return The message the user is currently typing, and if they want to send it or not
        inline user_chat chat(const std::vector<ca::message> &messages = {}) {
            auto chat = user_chat();

            ImGui::Begin("Chat");
            ImGui::Text("Chat");
            ImGui::Separator();

            // -5 is there to compensate for extra spacing between elements
            ImGui::BeginChild("messages_spacer", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5));

            const auto space = -ImGui::GetFrameHeightWithSpacing() -
                               messages.size() * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.y);
            ImGui::BeginChild("messages", ImVec2(0, space));
            ImGui::EndChild();
            for (const auto &msg : messages)
                if (msg.sent_by() == message::sender::other)
                    display_other_chat(msg);
                else
                    display_your_chat(msg);
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();

            ImGui::Separator();

            static auto message = std::array<char, 2049>(); // 2049 to allow for null terminator
            ImGui::Text("Message (2048 chars max)");
            ImGui::SameLine();
            ImGui::InputText("##MessageBox", message.data(), 2048);
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 40);
            chat.send = ImGui::Button("Send");
            chat.current_message = std::string(message.data());

            if (chat.send)
                message = std::array<char, 2049>();
            ImGui::End();
            return chat;
        }

        /// Handle the chat logic of processing new messages, sending, and reading them
        /// \param processor
        /// \param focused
        inline void handle_chat(ca::network_processor &processor, bool focused) {
            static auto messages = std::vector<ca::message>();

            // If there are incoming messages, add them to the stored messages
            if (const auto &incoming = processor.incoming_messages(); !incoming.empty())
                messages.insert(messages.end(), incoming.begin(), incoming.end());

            // Update the messages that have been read by the other client
            if (const auto &read_messages = processor.read_messages(); !read_messages.empty())
                for (auto &msg : messages)
                    for (auto hash : read_messages)
                        if (msg == hash) msg.set_seen();

            // If the window is focused, update the other client that we read the messages
            if (focused && !messages.empty())
                for (auto i = messages.size(); i >= 1; i--)
                {
                    auto &msg = messages[i - 1];
                    if (msg.sent_by() == ca::message::sender::other)
                    {
                        if (msg.seen()) break;
                        msg.set_seen();
                        processor.seen(msg.hash());
                    }
                }

            const auto chat = ui::chat(messages);

            if (chat.send && !chat.current_message.empty()) {
                const auto message = ca::message(chat.current_message);
                messages.emplace_back(message);
                processor.queue_message(message);
            }
        }

    }
}
