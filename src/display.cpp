#include "display.h"

ca::display::display() {
    glfwInit();

    glfwInitHint(GLFW_VERSION_MAJOR, 4);
    glfwInitHint(GLFW_VERSION_MINOR, 5);
    _window = glfwCreateWindow(1920, 1080, "Chat Application", nullptr, nullptr);

    glfwMakeContextCurrent(_window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    glfwSetWindowUserPointer(_window, this);
    glfwSetWindowFocusCallback(_window, [](GLFWwindow *window, int focused){
        auto display = static_cast<ca::display*>(glfwGetWindowUserPointer(window));
        display->_focused = focused;
    });
}

bool ca::display::running() const noexcept {
    return !glfwWindowShouldClose(_window);
}

ca::display::user_input ca::display::render(ca::network_processor &processor) const noexcept {
    auto input = user_input();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Main rendering code here
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto ui_ctx = ui::init();

    ui::root_node(ui_ctx);

    // Make user select mode to start up in
    if (processor.mode() == ca::client_mode::unknown)
    {
        ImGui::Begin("Select Mode", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Button("Server"))
            processor.set_mode(ca::client_mode::server);
        ImGui::SameLine();
        if (ImGui::Button("Client"))
            processor.set_mode(ca::client_mode::client);

        ImGui::End();
    }

    // Make user input an IP and PORT
    if (!processor.connected())
    {
        if (processor.mode() == ca::client_mode::client)
        {
            ImGui::Begin("Target Server");

            static auto address = std::array<char, 65>();
            ImGui::InputText("Address", address.data(), 64);

            static auto port = std::int32_t();
            ImGui::InputInt("Port", &port);

            if (ImGui::Button("Connect"))
                processor.connect(std::string(address.data()), port);

            ImGui::End();
        }
        else if (processor.mode() == ca::client_mode::server)
            processor.create_server();
    } else if (processor.waiting_on_connection())
    {
        ImGui::Begin("Server Information");
        ImGui::Text("Target port is %hu", processor.server_port());
        ImGui::End();

    } else {
        static auto messages = std::vector<ca::message>();

        for (const auto &msg : processor.incoming_messages())
            messages.push_back(msg);

        const auto read_messages = processor.read_messages();
        for (auto &msg : messages)
            for (auto hash : read_messages)
                if (msg == hash)
                    msg.set_seen();

        const auto user_chat = ui::chat(messages);

        if (_focused && !messages.empty())
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

        if (user_chat.send && !user_chat.current_message.empty())
        {
            const auto message = ca::message(user_chat.current_message);
            messages.emplace_back(message);
            processor.queue_message(message);
        }

    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
    glfwPollEvents();

    if (processor.waiting_on_connection())
    {
        static auto frame_wait = 0;
        if (frame_wait++ > 5)
            processor.wait_on_connection();
    }

    return input;
}
