#include "ApiServer.h"
#include "Kernel.h"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    std::cout
        << "========================================"
        << std::endl;

    std::cout
        << "          DRONE OS v1.0"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl
        << std::endl;

    DroneOS::Kernel kernel;

    if (!kernel.initialize())
    {
        std::cerr
            << "DroneOS initialization failed."
            << std::endl;

        return 1;
    }

    DroneOS::ApiServer apiServer(
        kernel,
        8080
    );

    if (!apiServer.start())
    {
        std::cerr
            << "API server failed to start."
            << std::endl;

        return 1;
    }

    kernel.start();

    std::cout
        << std::endl
        << "========================================"
        << std::endl;

    std::cout
        << "          DroneOS is ONLINE"
        << std::endl;

    std::cout
        << "          API: localhost:8080"
        << std::endl;

    std::cout
        << "========================================"
        << std::endl
        << std::endl;

    std::cout
        << "Available endpoints:"
        << std::endl;

    std::cout
        << "  /api/status"
        << std::endl;

    std::cout
        << "  /api/drone"
        << std::endl;

    std::cout
        << "  /api/telemetry"
        << std::endl;

    std::cout
        << "  /api/resources"
        << std::endl;

    std::cout
        << "  /api/tasks"
        << std::endl;

    std::cout
        << "  /api/mission"
        << std::endl;

    std::cout
        << std::endl;

    // Keep the OS alive.
    while (kernel.isRunning())
    {
        std::this_thread::sleep_for(
            std::chrono::seconds(1)
        );
    }

    apiServer.stop();
    kernel.stop();

    return 0;
}