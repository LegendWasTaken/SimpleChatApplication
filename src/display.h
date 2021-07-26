#pragma once

#include <string>
#include <optional>
#include <array>
#include <vector>

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

        struct user_input {
            bool sent;
            std::string message;
        };

        display();

        [[nodiscard]] bool running() const noexcept;

        [[nodiscard]] user_input render(ca::network_processor &processor) const noexcept;

    private:
        bool _focused;

        GLFWwindow *_window;
    };

    namespace ui {
        struct init_ctx {
            ImGuiDockNodeFlags dock_flags;
            ImGuiWindowFlags window_flags;
            ImGuiViewport *viewport;
        };

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

        inline void display_other_chat(const ca::message &message) {
            const auto time = message.local_time_sent();
            ImGui::Text("[%02hhu:%02hhu %s] - %s", time.hour, time.minute, time.am ? "AM" : "PM", message.content().c_str());
        }

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

    }
}
