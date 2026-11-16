
#include "Drone.h"

#include <iostream>
#include <vector>

int main()
{
    DroneOS::Drone drone;

    std::vector<DroneOS::Waypoint> route = {
        {28.6145, 77.2100, 125.0},
        {28.6150, 77.2110, 130.0},
        {28.6160, 77.2120, 120.0}
    };

    drone.setMission(route);
    drone.startMission();

    for (int i = 0; i < 2000; ++i)
    {
        drone.update();

        const auto& telemetry = drone.getTelemetry();

        std::cout
            << "Latitude: " << telemetry.latitude
            << " Longitude: " << telemetry.longitude
            << " Altitude: " << telemetry.altitude
            << " Battery: " << telemetry.battery
            << '\n';

        if (drone.getMissionState() ==
            DroneOS::MissionState::COMPLETED)
        {
            break;
        }
    }

    return 0;
}
