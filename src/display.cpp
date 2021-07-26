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

    // This allows us to hook into GLFW's window callbacks
    glfwSetWindowUserPointer(_window, this);
    glfwSetWindowFocusCallback(_window, [](GLFWwindow *window, int focused) {
        auto display = static_cast<ca::display *>(glfwGetWindowUserPointer(window));
        display->_focused = focused;
    });
}

bool ca::display::running() const noexcept {
    return !glfwWindowShouldClose(_window);
}

void ca::display::render(ca::network_processor &processor) const noexcept {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Main rendering code here
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto ui_ctx = ui::init();
    ui::root_node(ui_ctx);

    // Start screen will reply with a bool if it's finished prompting the user for stuff

    if (const auto error = processor.error(); !error.empty()) {
        if (ui::display_error(error))
            std::terminate();
    } else if (ui::display_start_screen(processor))
        ui::handle_chat(processor, _focused);


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(_window);
    glfwPollEvents();
}
