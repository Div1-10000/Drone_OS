#include "Kernel.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace DroneOS {

Kernel::Kernel()
    : droneId(-1),
      missionId(-1),
      running(false)
{
}

bool Kernel::initialize()
{
    std::cout
        << "[DRONE OS]"
        << std::endl;

    std::cout
        << "Kernel started"
        << std::endl;

    scheduler.initialize();

    resourceManager.initialize();

    faultManager.initialize();

    if (!database.initialize(
            "database/droneos.db"))
    {
        return false;
    }

    if (!database.createInitialDrone())
        return false;

    if (!database.createInitialMission())
        return false;

    droneId = database.getDroneId();
    missionId = database.getMissionId();

    if (droneId < 0 || missionId < 0)
    {
        std::cerr
            << "Failed to obtain database IDs."
            << std::endl;

        return false;
    }

    createTasks();

    configureDefaultMission();

    currentTelemetry =
        drone.getTelemetry();

    database.insertEvent(
        droneId,
        "SYSTEM",
        "DroneOS kernel initialized",
        "INFO"
    );

    return true;
}

void Kernel::createTasks()
{
    /*
     * Tasks are persistent DB records.
     *
     * On the first startup, create the six
     * standard DroneOS tasks.
     *
     * On subsequent restarts, reuse the
     * existing database records instead of
     * inserting duplicates.
     */
    if (database.hasTasksForMission(missionId))
    {
        std::cout
            << "[DATABASE] Existing mission tasks found"
            << std::endl;

        return;
    }

    struct TaskDefinition
    {
        const char* name;
        Priority priority;
        double cpu;
        double memory;
        double power;
    };

    const TaskDefinition definitions[] =
    {
        {
            "Navigation",
            Priority::CRITICAL,
            20.0,
            256.0,
            8.0
        },
        {
            "Obstacle Avoidance",
            Priority::CRITICAL,
            25.0,
            384.0,
            10.0
        },
        {
            "Telemetry",
            Priority::HIGH,
            10.0,
            128.0,
            4.0
        },
        {
            "Camera",
            Priority::HIGH,
            15.0,
            512.0,
            7.0
        },
        {
            "GPS",
            Priority::MEDIUM,
            8.0,
            128.0,
            3.0
        },
        {
            "Diagnostics",
            Priority::LOW,
            5.0,
            256.0,
            2.0
        }
    };

    if (!database.beginTransaction())
    {
        std::cerr
            << "[DATABASE] Failed to begin task transaction"
            << std::endl;

        return;
    }

    bool success = true;

    for (const auto& definition : definitions)
    {
        Task task;

        task.id = 0;
        task.name = definition.name;
        task.priority = definition.priority;
        task.state = TaskState::READY;
        task.cpuRequired = definition.cpu;
        task.memoryRequired = definition.memory;
        task.powerRequired = definition.power;

        const int databaseId =
            database.insertTask(
                missionId,
                task
            );

        if (databaseId <= 0)
        {
            success = false;
            break;
        }

        task.id = databaseId;

        scheduler.addTask(task);
    }

    if (!success)
    {
        database.rollbackTransaction();

        std::cerr
            << "[DATABASE] Task transaction rolled back"
            << std::endl;

        return;
    }

    if (!database.commitTransaction())
    {
        database.rollbackTransaction();

        std::cerr
            << "[DATABASE] Task transaction failed"
            << std::endl;

        return;
    }

    std::cout
        << "[DATABASE] Initial mission tasks committed"
        << std::endl;
}

void Kernel::runScheduler()
{
    Task* task = nullptr;
    {
        std::lock_guard<std::mutex> schedulerLock(schedulerMutex);
        task = scheduler.getNextTask();
        if (task == nullptr)
        {
            scheduler.resetCompletedTasks();
            task = scheduler.getNextTask();
        }
        if (task != nullptr)
            task->state = TaskState::RUNNING;
    }

    if (task == nullptr)
        return;

    database.updateTaskState(task->id, "RUNNING");
    database.insertEvent(
        droneId,
        "TASK_START",
        task->name + " started",
        "INFO"
    );

    std::cout << "[SCHEDULER] Running: " << task->name
              << " [" << priorityToString(task->priority) << "]"
              << std::endl;

    bool allocated = false;
    {
        std::lock_guard<std::mutex> resourceLock(resourceMutex);
        allocated = resourceManager.allocate(
            task->cpuRequired,
            task->memoryRequired,
            task->powerRequired
        );
    }

    if (!allocated)
    {
        {
            std::lock_guard<std::mutex> schedulerLock(schedulerMutex);
            scheduler.setWaiting(*task);
        }
        database.updateTaskState(task->id, "WAITING");
        return;
    }

    // Keep tasks observable while allowing frequent telemetry/resource samples.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::lock_guard<std::mutex> schedulerLock(schedulerMutex);
        scheduler.completeTask(*task);
    }
    database.updateTaskState(task->id, "COMPLETED");
    database.insertEvent(
        droneId,
        "TASK_COMPLETE",
        task->name + " completed",
        "INFO"
    );

    {
        std::lock_guard<std::mutex> resourceLock(resourceMutex);
        resourceManager.release(
            task->cpuRequired,
            task->memoryRequired,
            task->powerRequired
        );
    }
}

void Kernel::configureDefaultMission()
{
    const std::vector<Waypoint> waypoints =
    {
        {28.6145, 77.2098, 140.0},
        {28.6152, 77.2105, 150.0},
        {28.6143, 77.2110, 135.0},
        {28.6139, 77.2090, 120.0}
    };

    drone.setMission(waypoints);
}

void Kernel::start()
{
    if (running)
        return;

    running = true;

    simulationThread =
        std::thread(
            &Kernel::simulationLoop,
            this
        );
}

void Kernel::simulationLoop()
{
    std::cout
        << "[KERNEL] Simulation engine started"
        << std::endl;

    unsigned long sample = 0;
    while (running)
    {
        ++sample;
        /*
         * Execute one scheduled OS task.
         *
         * Resources are allocated while
         * the task is running and released
         * after completion.
         */
        runScheduler();

        Telemetry telemetry{};
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            drone.update();
            telemetry = drone.getTelemetry();
            currentTelemetry = telemetry;
        }
            
        const double cpuUsage =
            std::clamp(
                20.0 +
                telemetry.speed * 1.2 +
                telemetry.altitude * 0.05 +
                static_cast<double>(static_cast<int>(sample % 9) - 4) * 1.15,
                5.0,
                90.0
            );
        
        const double memoryUsagePercent =
            std::clamp(
                25.0 +
                telemetry.altitude * 0.08 +
                telemetry.speed * 0.5 +
                static_cast<double>(static_cast<int>(sample % 7) - 3) * 0.65,
                10.0,
                85.0
            );
        const double memoryUsage =
            memoryUsagePercent * 8192.0 / 100.0;
        
        const double powerUsage =
            std::clamp(
                100.0 +
                telemetry.speed * 4.0 +
                telemetry.altitude * 0.35 +
                static_cast<double>(static_cast<int>(sample % 11) - 5) * 2.0,
                80.0,
                550.0
            );
        
        const double storageUsage =
            std::clamp(
                30.0 +
                telemetry.altitude * 0.15 +
                static_cast<double>(static_cast<int>((sample / 4) % 5) - 2) * 0.25,
                10.0,
                95.0
            );
        
        {
            std::lock_guard<std::mutex> resourceLock(resourceMutex);
            resourceManager.updateDynamicUsage(
                cpuUsage,
                memoryUsage,
                powerUsage,
                storageUsage
            );
        }
        database.insertTelemetry(
            droneId,
            telemetry
        );

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            faultManager.evaluate(telemetry);
        }

        bool activeFault = false;
        std::string faultMode;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            activeFault = faultManager.hasActiveFault();
            faultMode = faultManager.getModeString();
        }
        if (activeFault)
        {
            database.insertEvent(
                droneId,
                "FAULT",
                "Flight mode changed to " + faultMode,
                "WARNING"
            );
        }

        std::cout
            << "[TELEMETRY] "
            << "ALT="
            << telemetry.altitude
            << "m | SPD="
            << telemetry.speed
            << "m/s | BAT="
            << telemetry.battery
            << "% | SIG="
            << telemetry.signal
            << "%"
            << std::endl;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(200)
        );
    }
}

void Kernel::stop()
{
    if (!running)
        return;

    running = false;

    if (simulationThread.joinable())
        simulationThread.join();

    database.insertEvent(
        droneId,
        "SYSTEM",
        "DroneOS kernel stopped",
        "INFO"
    );
}

bool Kernel::isRunning() const
{
    return running;
}

Telemetry Kernel::getTelemetry() const
{
    std::lock_guard<std::mutex> lock(
        stateMutex
    );

    return currentTelemetry;
}

std::string Kernel::getFlightMode() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return faultManager.getModeString();
}

bool Kernel::hasFault() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return faultManager.hasActiveFault();
}

double Kernel::getCpuUsage() const
{
    std::lock_guard<std::mutex> lock(resourceMutex);
    return resourceManager.getCpuUsage();
}

double Kernel::getMemoryUsage() const
{
    std::lock_guard<std::mutex> lock(resourceMutex);
    return resourceManager.getMemoryUsage();
}

double Kernel::getPowerUsage() const
{
    std::lock_guard<std::mutex> lock(resourceMutex);
    return resourceManager.getPowerUsage();
}

double Kernel::getStorageUsage() const
{
    std::lock_guard<std::mutex> lock(resourceMutex);
    return resourceManager.getStorageUsage();
}

const Scheduler& Kernel::getScheduler() const
{
    return scheduler;
}

std::vector<Task> Kernel::getTasksSnapshot() const
{
    std::lock_guard<std::mutex> lock(schedulerMutex);
    return scheduler.getTasks();
}

bool Kernel::startMission()
{
    std::lock_guard<std::mutex> lock(stateMutex);

    const MissionState state = drone.getMissionState();
    if (state == MissionState::RUNNING || state == MissionState::RETURNING)
        return true;

    if (state == MissionState::PAUSED)
        return false;

    if (state == MissionState::COMPLETED || drone.getTotalWaypoints() == 0)
        configureDefaultMission();

    drone.startMission();
    currentTelemetry = drone.getTelemetry();
    return drone.getMissionState() == MissionState::RUNNING;
}

bool Kernel::pauseMission()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (drone.getMissionState() != MissionState::RUNNING &&
        drone.getMissionState() != MissionState::RETURNING)
        return false;

    drone.pauseMission();
    currentTelemetry = drone.getTelemetry();
    return drone.getMissionState() == MissionState::PAUSED;
}

bool Kernel::resumeMission()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (drone.getMissionState() != MissionState::PAUSED)
        return false;

    drone.resumeMission();
    currentTelemetry = drone.getTelemetry();
    return drone.getMissionState() == MissionState::RUNNING ||
           drone.getMissionState() == MissionState::RETURNING;
}

bool Kernel::resetMission()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    drone.resetMission();
    configureDefaultMission();
    currentTelemetry = drone.getTelemetry();
    return drone.getMissionState() == MissionState::IDLE;
}

std::string Kernel::getMissionState() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    switch (drone.getMissionState())
    {
        case MissionState::IDLE: return "IDLE";
        case MissionState::RUNNING: return "RUNNING";
        case MissionState::PAUSED: return "PAUSED";
        case MissionState::RETURNING: return "RETURNING";
        case MissionState::COMPLETED: return "COMPLETED";
    }
    return "UNKNOWN";
}

double Kernel::getMissionProgress() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    const MissionState state = drone.getMissionState();
    if (state == MissionState::COMPLETED)
        return 100.0;

    const std::size_t total = drone.getTotalWaypoints();
    if (total == 0 || state == MissionState::IDLE)
        return 0.0;

    const double progress =
        100.0 * static_cast<double>(drone.getCurrentWaypointIndex()) /
        static_cast<double>(total);
    return std::clamp(progress, 0.0, 100.0);
}

std::size_t Kernel::getCurrentWaypoint() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    const MissionState state = drone.getMissionState();
    const std::size_t total = drone.getTotalWaypoints();
    const std::size_t current = drone.getCurrentWaypointIndex();
    if (state == MissionState::IDLE || total == 0)
        return 0;
    if (state == MissionState::COMPLETED || current >= total)
        return total;
    return current + 1;
}

std::size_t Kernel::getTotalWaypoints() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return drone.getTotalWaypoints();
}

int Kernel::getDroneId() const
{
    return droneId;
}

int Kernel::getMissionId() const
{
    return missionId;
}

std::string Kernel::getDatabaseRecordsJson() const
{
    return const_cast<Database&>(database).getDashboardRecordsJson();
}

int Kernel::assignFleetTask(const std::string& droneName, const std::string& taskName)
{
    if (droneName.empty() || taskName.empty()) return -1;
    return database.assignFleetTask(droneName, taskName);
}

bool Kernel::updateFleetTaskState(int taskId, const std::string& state)
{
    if (taskId <= 0 || (state != "IN_PROGRESS" && state != "COMPLETED" && state != "FAILED"))
        return false;
    return database.updateTaskState(taskId, state);
}

}