#ifndef DRONEOS_DATABASE_H
#define DRONEOS_DATABASE_H

#include <sqlite3.h>

#include <string>

#include "Drone.h"
#include "Scheduler.h"

namespace DroneOS {

class Database
{
private:
    sqlite3* db;

public:
    Database();

    bool initialize(
        const std::string& databasePath
    );

    bool execute(
        const std::string& sql
    );

    bool executeSchema();

    int getDroneId();

    int getMissionId();

    bool createInitialDrone();

    bool createInitialMission();

    std::string getDashboardRecordsJson();

    int assignFleetTask(const std::string& droneName, const std::string& taskName);

    bool hasTasksForMission(
        int missionId
    );

    bool beginTransaction();

    bool commitTransaction();

    bool rollbackTransaction();

    int insertTask(
        int missionId,
        const Task& task
    );

    bool updateTaskState(
        int taskId,
        const std::string& state
    );

    bool insertTelemetry(
        int droneId,
        const Telemetry& telemetry
    );

    bool insertEvent(
        int droneId,
        const std::string& type,
        const std::string& message,
        const std::string& severity
    );

    bool insertFault(
        int droneId,
        const std::string& type,
        const std::string& severity,
        const std::string& description
    );

    bool resolveFault(
        int faultId
    );

    void shutdown();

    ~Database();
};

}

#endif