
#ifndef DRONEOS_DRONE_H
#define DRONEOS_DRONE_H

#include <cstddef>
#include <chrono>
#include <vector>

namespace DroneOS {

struct Telemetry
{
    double altitude;
    double speed;
    double battery;
    double signal;
    double temperature;
    double latitude;
    double longitude;
};

struct Waypoint
{
    double latitude;
    double longitude;
    double altitude;
};

enum class MissionState
{
    IDLE,
    RUNNING,
    PAUSED,
    RETURNING,
    COMPLETED
};

class Drone
{
private:
    Telemetry telemetry;

    std::vector<Waypoint> missionWaypoints;
    std::size_t currentWaypointIndex;

    MissionState missionState;
    MissionState pausedFromState;

    double homeLatitude;
    double homeLongitude;
    double homeAltitude;
    std::chrono::steady_clock::time_point lastMovementTime;

    void moveToward(const Waypoint& target);

public:
    Drone();

    // Existing telemetry API
    void update();
    const Telemetry& getTelemetry() const;

    bool isBatteryLow() const;
    bool isSignalWeak() const;
    bool isOverheating() const;

    // Mission management
void setMission(const std::vector<Waypoint>& waypoints);
void startMission();
void pauseMission();
void resumeMission();
void returnHome();
void resetMission();

MissionState getMissionState() const;
std::size_t getCurrentWaypointIndex() const;
std::size_t getTotalWaypoints() const;
};

}

#endif
