#ifndef DRONEOS_API_SERVER_H
#define DRONEOS_API_SERVER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <cstdint>

namespace DroneOS {

class Kernel;

class ApiServer
{
private:
    Kernel& kernel;

    std::atomic<bool> running;

    std::thread serverThread;

    mutable std::mutex startupMutex;
    std::condition_variable startupCondition;
    bool startupComplete;
    bool startupSuccess;

    int port;

    void serverLoop();

    void handleClient(
        std::uint64_t clientSocket
    );

    std::string route(
        const std::string& path,
        const std::string& body = {}
    );

    static std::string jsonEscape(
        const std::string& value
    );

public:
    ApiServer(
        Kernel& kernel,
        int port = 8080
    );

    bool start();

    void stop();

    ~ApiServer();
};

}

#endif