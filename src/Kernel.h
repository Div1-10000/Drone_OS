#ifndef DRONEOS_KERNEL_H
#define DRONEOS_KERNEL_H

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <cstddef>
#include <vector>

#include "Database.h"
#include "Drone.h"
#include "FaultManager.h"
#include "ResourceManager.h"
#include "Scheduler.h"

namespace DroneOS {

class Kernel
{
private:
    Scheduler scheduler;
    ResourceManager resourceManager;
    Drone drone;
    Database database;
    FaultManager faultManager;

    Telemetry currentTelemetry;

    int droneId;
    int missionId;

    std::atomic<bool> running;

    std::thread simulationThread;

    mutable std::mutex stateMutex;
    mutable std::mutex schedulerMutex;
    mutable std::mutex resourceMutex;

    void simulationLoop();

    void createTasks();

    void runScheduler();

    void configureDefaultMission();

public:
    Kernel();

    bool initialize();

    void start();

    void stop();

    bool isRunning() const;

    Telemetry getTelemetry() const;

    std::string getFlightMode() const;

    bool hasFault() const;

    double getCpuUsage() const;

    double getMemoryUsage() const;

    double getPowerUsage() const;

    double getStorageUsage() const;

    const Scheduler& getScheduler() const;
    std::vector<Task> getTasksSnapshot() const;

    bool startMission();
    bool pauseMission();
    bool resumeMission();
    bool resetMission();

    std::string getMissionState() const;
    double getMissionProgress() const;
    std::size_t getCurrentWaypoint() const;
    std::size_t getTotalWaypoints() const;

    int getDroneId() const;

    int getMissionId() const;

    std::string getDatabaseRecordsJson() const;

    int assignFleetTask(const std::string& droneName, const std::string& taskName);

    bool updateFleetTaskState(int taskId, const std::string& state);
};

}

#endif