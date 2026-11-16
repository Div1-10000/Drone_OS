#include "ApiServer.h"
#include "Kernel.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace DroneOS {

ApiServer::ApiServer(
    Kernel& kernel,
    int port)
    : kernel(kernel),
      running(false),
      startupComplete(false),
      startupSuccess(false),
      port(port)
{
}

std::string ApiServer::jsonEscape(
    const std::string& value)
{
    std::string result;

    for (char c : value)
    {
        switch (c)
        {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:
                result += c;
                break;
        }
    }

    return result;
}

std::string ApiServer::route(
    const std::string& path,
    const std::string& body)
{
    const Telemetry telemetry =
        kernel.getTelemetry();

    std::ostringstream json;

    json << std::fixed
         << std::setprecision(6);

    if (path == "/api/tasks/assign")
    {
        auto field = [&body](const std::string& key) {
            const std::string token = "\"" + key + "\"";
            std::size_t pos = body.find(token);
            if (pos == std::string::npos) return std::string();
            pos = body.find(':', pos + token.size());
            if (pos == std::string::npos) return std::string();
            pos = body.find('"', pos + 1);
            if (pos == std::string::npos) return std::string();
            const std::size_t end = body.find('"', pos + 1);
            if (end == std::string::npos) return std::string();
            return body.substr(pos + 1, end - pos - 1);
        };
        const std::string droneName = field("drone");
        const std::string taskName = field("task");
        const int taskId = kernel.assignFleetTask(droneName, taskName);
        const bool ok = taskId > 0;
        json << "{\"success\":" << (ok ? "true" : "false")
             << ",\"task_id\":" << (ok ? taskId : 0)
             << ",\"message\":\"" << (ok ? "Task saved for " + droneName : "Invalid drone or task") << "\"}";
        return json.str();
    }

    if (path == "/api/tasks/state")
    {
        auto field = [&body](const std::string& key) {
            const std::string token = "\"" + key + "\"";
            std::size_t pos = body.find(token);
            if (pos == std::string::npos) return std::string();
            pos = body.find(':', pos + token.size());
            if (pos == std::string::npos) return std::string();
            pos = body.find('\"', pos + 1);
            if (pos == std::string::npos) return std::string();
            const std::size_t end = body.find('\"', pos + 1);
            if (end == std::string::npos) return std::string();
            return body.substr(pos + 1, end - pos - 1);
        };
        const int taskId = std::atoi(field("task_id").c_str());
        const bool ok = kernel.updateFleetTaskState(taskId, field("state"));
        json << "{\"success\":" << (ok ? "true" : "false") << "}";
        return json.str();
    }

    if (path == "/api/mission/start")
    {
        const bool ok = kernel.startMission();
        json << "{\"success\":" << (ok ? "true" : "false")
             << ",\"message\":\""
             << (ok ? "Mission started" : "Mission could not be started")
             << "\"}";
        return json.str();
    }
    if (path == "/api/mission/pause")
    {
        const bool ok = kernel.pauseMission();
        json << "{\"success\":" << (ok ? "true" : "false")
             << ",\"message\":\""
             << (ok ? "Mission paused" : "Mission is not running")
             << "\"}";
        return json.str();
    }
    if (path == "/api/mission/resume")
    {
        const bool ok = kernel.resumeMission();
        json << "{\"success\":" << (ok ? "true" : "false")
             << ",\"message\":\""
             << (ok ? "Mission resumed" : "Mission is not paused")
             << "\"}";
        return json.str();
    }
    if (path == "/api/mission/reset")
    {
        const bool ok = kernel.resetMission();
        json << "{\"success\":" << (ok ? "true" : "false")
             << ",\"message\":\""
             << (ok ? "Mission reset" : "Mission reset failed")
             << "\"}";
        return json.str();
    }

    /*
     * ========================================================
     * SYSTEM STATUS
     * ========================================================
     */

    if (path == "/api/status")
    {
        json
            << "{"
            << "\"status\":\""
            << (kernel.isRunning()
                    ? "ONLINE"
                    : "OFFLINE")
            << "\","
            << "\"flight_mode\":\""
            << jsonEscape(
                kernel.getFlightMode()
            )
            << "\","
            << "\"fault\":"
            << (kernel.hasFault()
                    ? "true"
                    : "false")
            << "}";
    }

    /*
     * ========================================================
     * DRONE
     * ========================================================
     */

    else if (path == "/api/drone")
    {
        json
            << "{"
            << "\"drone_id\":"
            << kernel.getDroneId()
            << ","
            << "\"name\":\"DRONE-X1\","
            << "\"status\":\""
            << (kernel.isRunning()
                    ? "ONLINE"
                    : "OFFLINE")
            << "\","
            << "\"altitude\":"
            << telemetry.altitude
            << ","
            << "\"speed\":"
            << telemetry.speed
            << ","
            << "\"battery\":"
            << telemetry.battery
            << ","
            << "\"signal\":"
            << telemetry.signal
            << ","
            << "\"temperature\":"
            << telemetry.temperature
            << ","
            << "\"latitude\":"
            << telemetry.latitude
            << ","
            << "\"longitude\":"
            << telemetry.longitude
            << ","
            << "\"flight_mode\":\""
            << jsonEscape(
                kernel.getFlightMode()
            )
            << "\""
            << "}";
    }

    /*
     * ========================================================
     * LIVE TELEMETRY
     * ========================================================
     */

    else if (path == "/api/telemetry")
    {
        json
            << "{"
            << "\"altitude\":"
            << telemetry.altitude
            << ","
            << "\"speed\":"
            << telemetry.speed
            << ","
            << "\"battery\":"
            << telemetry.battery
            << ","
            << "\"signal\":"
            << telemetry.signal
            << ","
            << "\"temperature\":"
            << telemetry.temperature
            << ","
            << "\"latitude\":"
            << telemetry.latitude
            << ","
            << "\"longitude\":"
            << telemetry.longitude
            << "}";
    }

    /*
     * ========================================================
     * RESOURCE MANAGER
     * ========================================================
     */

    else if (path == "/api/resources")
    {
        json
            << "{"
            << "\"cpu\":"
            << kernel.getCpuUsage()
            << ","
            << "\"memory\":"
            << kernel.getMemoryUsage()
            << ","
            << "\"power\":"
            << kernel.getPowerUsage()
            << ","
            << "\"storage\":"
            << kernel.getStorageUsage()
            << "}";
    }

    /*
     * ========================================================
     * SCHEDULER TASKS
     * ========================================================
     */

    else if (path == "/api/tasks")
    {
        json << "[";

        const auto tasks = kernel.getTasksSnapshot();

        for (std::size_t i = 0;
             i < tasks.size();
             ++i)
        {
            const Task& task = tasks[i];

            if (i > 0)
                json << ",";

            json
                << "{"
                << "\"id\":"
                << task.id
                << ","
                << "\"name\":\""
                << jsonEscape(task.name)
                << "\","
                << "\"priority\":\""
                << priorityToString(
                    task.priority
                )
                << "\","
                << "\"state\":\""
                << taskStateToString(
                    task.state
                )
                << "\","
                << "\"cpu\":"
                << task.cpuRequired
                << ","
                << "\"memory\":"
                << task.memoryRequired
                << ","
                << "\"power\":"
                << task.powerRequired
                << "}";
        }

        json << "]";
    }

    /*
     * ========================================================
     * MISSION
     * ========================================================
     */

    else if (path == "/api/mission")
    {
        json
            << "{"
            << "\"mission_id\":" << kernel.getMissionId() << ","
            << "\"name\":\"AUTONOMOUS SURVEY\","
            << "\"status\":\"" << kernel.getMissionState() << "\","
            << "\"progress\":" << kernel.getMissionProgress() << ","
            << "\"current_waypoint\":" << kernel.getCurrentWaypoint() << ","
            << "\"total_waypoints\":" << kernel.getTotalWaypoints()
            << "}";
    }

    else if (path == "/api/database")
    {
        return "{\"status\":\"CONNECTED\","
               "\"engine\":\"SQLite\","
               "\"database\":\"database/droneos.db\","
               "\"records\":" + kernel.getDatabaseRecordsJson() + "}";
    }

    /*
     * ========================================================
     * UNKNOWN ENDPOINT
     * ========================================================
     */

    else
    {
        return "";
    }

    return json.str();
}


/*
 * ============================================================
 * CLIENT HANDLER
 * ============================================================
 */

void ApiServer::handleClient(std::uint64_t clientSocket)
{
    SOCKET client = static_cast<SOCKET>(clientSocket);
    char buffer[8192] = {};
    const int received = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0)
    {
        closesocket(client);
        return;
    }

    std::istringstream requestStream(std::string(buffer, received));
    std::string method, path, version;
    requestStream >> method >> path >> version;

    const std::size_t queryPosition = path.find('?');
    if (queryPosition != std::string::npos)
        path.erase(queryPosition);

    auto sendAll = [client](const std::string& data)
    {
        std::size_t sent = 0;
        while (sent < data.size())
        {
            const int result = send(
                client,
                data.data() + sent,
                static_cast<int>(data.size() - sent),
                0
            );
            if (result == SOCKET_ERROR || result == 0)
                break;
            sent += static_cast<std::size_t>(result);
        }
    };

    auto respond = [&sendAll](int status, const std::string& reason,
                              const std::string& body, bool includeBody = true)
    {
        std::ostringstream response;
        response << "HTTP/1.1 " << status << " " << reason << "\r\n"
                 << "Content-Type: application/json; charset=utf-8\r\n"
                 << "Access-Control-Allow-Origin: *\r\n"
                 << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                 << "Access-Control-Allow-Headers: Content-Type, Accept\r\n"
                 << "Cache-Control: no-store\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n";
        if (includeBody)
            response << body;
        sendAll(response.str());
    };

    if (method == "OPTIONS")
    {
        respond(204, "No Content", "", false);
        closesocket(client);
        return;
    }

    const bool controlPath =
        path == "/api/mission/start" ||
        path == "/api/mission/pause" ||
        path == "/api/mission/resume" ||
        path == "/api/mission/reset" ||
        path == "/api/tasks/assign" ||
        path == "/api/tasks/state";

    if (method != "GET" && method != "POST")
    {
        respond(405, "Method Not Allowed", "{\"error\":\"Method not allowed\"}");
        closesocket(client);
        return;
    }

    if ((method == "POST" && !controlPath) ||
        (method == "GET" && controlPath))
    {
        respond(405, "Method Not Allowed", "{\"error\":\"Use POST for mission controls\"}");
        closesocket(client);
        return;
    }

    std::string requestBody;
    const std::string requestText(buffer, received);
    const std::size_t bodyStart = requestText.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
        requestBody = requestText.substr(bodyStart + 4);

    const std::string responseBody = route(path, requestBody);
    if (responseBody.empty())
        respond(404, "Not Found", "{\"error\":\"Endpoint not found\"}");
    else
        respond(200, "OK", responseBody);

    closesocket(client);
}


/*
 * ============================================================
 * SERVER LOOP
 * ============================================================
 */

void ApiServer::serverLoop()
{
    SOCKET serverSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

    if (serverSocket ==
        INVALID_SOCKET)
    {
        std::cerr
            << "[API] Socket creation failed."
            << std::endl;
        {
            std::lock_guard<std::mutex> lock(startupMutex);
            startupComplete = true;
            startupSuccess = false;
        }
        startupCondition.notify_all();
        return;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        htonl(INADDR_ANY);

    serverAddress.sin_port =
        htons(
            static_cast<u_short>(port)
        );


    /*
     * Allow immediate restart of the server.
     */

    BOOL reuseAddress = TRUE;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(
            &reuseAddress
        ),
        sizeof(reuseAddress)
    );


    if (bind(
            serverSocket,
            reinterpret_cast<
                sockaddr*
            >(&serverAddress),
            sizeof(serverAddress)
        ) == SOCKET_ERROR)
    {
        std::cerr
            << "[API] Bind failed."
            << std::endl;

        closesocket(serverSocket);
        {
            std::lock_guard<std::mutex> lock(startupMutex);
            startupComplete = true;
            startupSuccess = false;
        }
        startupCondition.notify_all();
        return;
    }


    if (listen(
            serverSocket,
            SOMAXCONN
        ) == SOCKET_ERROR)
    {
        std::cerr
            << "[API] Listen failed."
            << std::endl;

        closesocket(serverSocket);
        {
            std::lock_guard<std::mutex> lock(startupMutex);
            startupComplete = true;
            startupSuccess = false;
        }
        startupCondition.notify_all();
        return;
    }


    {
        std::lock_guard<std::mutex> lock(startupMutex);
        startupComplete = true;
        startupSuccess = true;
    }
    startupCondition.notify_all();

    std::cout
        << "[API] Server listening on "
        << "http://localhost:"
        << port
        << std::endl;


    while (running)
    {
        SOCKET client =
            accept(
                serverSocket,
                nullptr,
                nullptr
            );

        if (client ==
            INVALID_SOCKET)
        {
            if (running)
            {
                std::cerr
                    << "[API] Accept failed."
                    << std::endl;
            }

            break;
        }

        handleClient(
            static_cast<uintptr_t>(
                client
            )
        );
    }


    closesocket(serverSocket);
}


/*
 * ============================================================
 * START
 * ============================================================
 */

bool ApiServer::start()
{
    if (running)
        return true;

    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "[API] WSAStartup failed." << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(startupMutex);
        startupComplete = false;
        startupSuccess = false;
    }
    running = true;
    serverThread = std::thread(&ApiServer::serverLoop, this);

    bool started = false;
    {
        std::unique_lock<std::mutex> lock(startupMutex);
        const bool signaled = startupCondition.wait_for(
            lock,
            std::chrono::seconds(3),
            [this] { return startupComplete; }
        );
        started = signaled && startupSuccess;
    }

    if (!started)
    {
        running = false;
        if (serverThread.joinable())
            serverThread.join();
        WSACleanup();
        return false;
    }

    return true;
}

/*
 * ============================================================
 * STOP
 * ============================================================
 */

void ApiServer::stop()
{
    if (!running)
        return;


    running = false;


    /*
     * Wake blocking accept().
     */

    SOCKET wakeSocket =
        socket(
            AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP
        );

    if (wakeSocket !=
        INVALID_SOCKET)
    {
        sockaddr_in address{};

        address.sin_family =
            AF_INET;

        address.sin_port =
            htons(
                static_cast<u_short>(port)
            );

        inet_pton(
            AF_INET,
            "127.0.0.1",
            &address.sin_addr
        );

        connect(
            wakeSocket,
            reinterpret_cast<
                sockaddr*
            >(&address),
            sizeof(address)
        );

        closesocket(wakeSocket);
    }


    if (serverThread.joinable())
        serverThread.join();


    WSACleanup();
}


/*
 * ============================================================
 * DESTRUCTOR
 * ============================================================
 */

ApiServer::~ApiServer()
{
    stop();
}

} // namespace DroneOS