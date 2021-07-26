#include <display.h>
#include <network_processor.h>

int main() {
    sockpp::socket_initializer initializer;

    auto network_processor = ca::network_processor();

    const auto display = ca::display();
    while (display.running())
        display.render(network_processor);
    return 0;
}
