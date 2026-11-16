
#include "Drone.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace DroneOS {

namespace {
constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_RADIUS_METERS = 6371000.0;

// Distance covered during each actual movement step.
constexpr double MOVEMENT_STEP_METERS = 15.0;

// Minimum time between movement steps.
constexpr auto MOVEMENT_INTERVAL = std::chrono::seconds(2);

double toRadians(double degrees)
{
    return degrees * PI / 180.0;
}
}

Drone::Drone()
    : currentWaypointIndex(0),
      missionState(MissionState::IDLE),
      pausedFromState(MissionState::IDLE)
{
    telemetry.altitude = 120.0;
    telemetry.speed = 15.0;
    telemetry.battery = 100.0;
    telemetry.signal = 98.0;
    telemetry.temperature = 32.0;
    telemetry.latitude = 28.6139;
    telemetry.longitude = 77.2090;

    homeLatitude = telemetry.latitude;
    homeLongitude = telemetry.longitude;
    homeAltitude = telemetry.altitude;
    lastMovementTime = std::chrono::steady_clock::now();

    std::cout << "Drone initialized" << std::endl;
}

void Drone::setMission(const std::vector<Waypoint>& waypoints)
{
    missionWaypoints = waypoints;
    currentWaypointIndex = 0;

    missionState = MissionState::IDLE;
    pausedFromState = MissionState::IDLE;
}

void Drone::startMission()
{
    if (missionWaypoints.empty())
    {
        std::cout << "Cannot start mission: no waypoints."
                  << std::endl;
        return;
    }

    if (telemetry.battery <= 0.0)
    {
        std::cout << "Cannot start mission: battery depleted."
                  << std::endl;
        return;
    }

    currentWaypointIndex = 0;
    missionState = MissionState::RUNNING;

    std::cout << "Mission started." << std::endl;
}

void Drone::pauseMission()
{
    if (missionState == MissionState::RUNNING ||
        missionState == MissionState::RETURNING)
    {
        pausedFromState = missionState;
        missionState = MissionState::PAUSED;

        std::cout << "Mission paused." << std::endl;
    }
}

void Drone::resumeMission()
{
    if (missionState == MissionState::PAUSED)
    {
        missionState = pausedFromState;

        std::cout << "Mission resumed." << std::endl;
    }
}

void Drone::returnHome()
{
    missionWaypoints.clear();

    missionWaypoints.push_back({
        homeLatitude,
        homeLongitude,
        homeAltitude
    });

    currentWaypointIndex = 0;
    missionState = MissionState::RETURNING;

    std::cout << "Return-to-home initiated." << std::endl;
}

void Drone::resetMission()
{
    missionWaypoints.clear();
    currentWaypointIndex = 0;

    missionState = MissionState::IDLE;
    pausedFromState = MissionState::IDLE;

    telemetry.latitude = homeLatitude;
    telemetry.longitude = homeLongitude;
    telemetry.altitude = homeAltitude;
    telemetry.speed = 0.0;
    telemetry.battery = 100.0;

    std::cout << "Mission reset." << std::endl;
}

void Drone::moveToward(const Waypoint& target)
{
    const double lat1 = toRadians(telemetry.latitude);
    const double lat2 = toRadians(target.latitude);
    const double lon1 = toRadians(telemetry.longitude);
    const double lon2 = toRadians(target.longitude);

    const double x =
        (lon2 - lon1) * std::cos((lat1 + lat2) / 2.0);

    const double y = lat2 - lat1;

    const double distance =
        EARTH_RADIUS_METERS * std::sqrt(x * x + y * y);

    if (distance <= MOVEMENT_STEP_METERS)
    {
        telemetry.latitude = target.latitude;
        telemetry.longitude = target.longitude;
        telemetry.altitude = target.altitude;
        return;
    }

    const double fraction = MOVEMENT_STEP_METERS / distance;

    telemetry.latitude +=
        (target.latitude - telemetry.latitude) * fraction;

    telemetry.longitude +=
        (target.longitude - telemetry.longitude) * fraction;

    telemetry.altitude +=
        (target.altitude - telemetry.altitude) * fraction;
}

void Drone::update()
{
    telemetry.signal = std::clamp(
        90.0 + (std::rand() % 100) / 10.0,
        0.0,
        100.0
    );

    telemetry.temperature = std::clamp(
        telemetry.temperature +
            ((std::rand() % 21) - 10) / 100.0,
        28.0,
        45.0
    );

    // Keep the drone stationary when no mission is active.
    if (missionState != MissionState::RUNNING &&
        missionState != MissionState::RETURNING)
    {
        telemetry.speed = 0.0;
        return;
    }

    telemetry.speed = 7.5;
    telemetry.battery = std::max(
        0.0,
        telemetry.battery - 0.01
    );

    if (telemetry.battery <= 0.0)
    {
        pausedFromState = missionState;
        missionState = MissionState::PAUSED;
        telemetry.speed = 0.0;

        std::cout << "Mission paused: battery depleted."
                  << std::endl;
        return;
    }

    if (currentWaypointIndex >= missionWaypoints.size())
    {
        missionState = MissionState::COMPLETED;
        telemetry.speed = 0.0;
        return;
    }

    // Limit movement frequency so the mission remains controllable.
    const auto now = std::chrono::steady_clock::now();

    if (now - lastMovementTime < MOVEMENT_INTERVAL)
        return;

    lastMovementTime = now;

    const Waypoint& target =
        missionWaypoints[currentWaypointIndex];

    moveToward(target);

    const double latDifference =
        (target.latitude - telemetry.latitude) * 111320.0;

    const double lonDifference =
        (target.longitude - telemetry.longitude) *
        111320.0 *
        std::cos(toRadians(telemetry.latitude));

    const double remainingDistance = std::sqrt(
        latDifference * latDifference +
        lonDifference * lonDifference
    );

    if (remainingDistance < 0.5)
    {
        telemetry.latitude = target.latitude;
        telemetry.longitude = target.longitude;
        telemetry.altitude = target.altitude;

        ++currentWaypointIndex;

        if (currentWaypointIndex >= missionWaypoints.size())
        {
            missionState = MissionState::COMPLETED;
            telemetry.speed = 0.0;

            std::cout << "Mission completed." << std::endl;
        }
    }
}

const Telemetry& Drone::getTelemetry() const
{
    return telemetry;
}

bool Drone::isBatteryLow() const
{
    return telemetry.battery < 20.0;
}

bool Drone::isSignalWeak() const
{
    return telemetry.signal < 30.0;
}

bool Drone::isOverheating() const
{
    return telemetry.temperature > 70.0;
}

MissionState Drone::getMissionState() const
{
    return missionState;
}

std::size_t Drone::getCurrentWaypointIndex() const
{
    return currentWaypointIndex;
}

std::size_t Drone::getTotalWaypoints() const
{
    return missionWaypoints.size();
}

}
