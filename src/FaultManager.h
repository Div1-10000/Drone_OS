#ifndef DRONEOS_FAULT_MANAGER_H
#define DRONEOS_FAULT_MANAGER_H

#include <string>

#include "Drone.h"

namespace DroneOS {

enum class FlightMode
{
    NORMAL,
    SAFE_MODE,
    RETURN_TO_HOME
};

class FaultManager
{
private:
    FlightMode mode;

    bool batteryFaultActive;
    bool signalFaultActive;
    bool temperatureFaultActive;

public:
    FaultManager();

    void initialize();

    void evaluate(
        const Telemetry& telemetry
    );

    FlightMode getMode() const;

    bool hasActiveFault() const;

    std::string getModeString() const;
};

}

#endif