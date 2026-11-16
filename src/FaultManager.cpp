#include "FaultManager.h"

#include <iostream>

namespace DroneOS {

FaultManager::FaultManager()
    : mode(FlightMode::NORMAL),
      batteryFaultActive(false),
      signalFaultActive(false),
      temperatureFaultActive(false)
{
}

void FaultManager::initialize()
{
    std::cout
        << "Fault Manager started"
        << std::endl;
}

void FaultManager::evaluate(
    const Telemetry& telemetry)
{
    batteryFaultActive =
        telemetry.battery < 20.0;

    signalFaultActive =
        telemetry.signal < 30.0;

    temperatureFaultActive =
        telemetry.temperature > 70.0;

    if (temperatureFaultActive)
    {
        mode = FlightMode::SAFE_MODE;

        std::cout
            << "[FAULT] OVER_TEMPERATURE"
            << std::endl;

        return;
    }

    if (batteryFaultActive)
    {
        mode = FlightMode::RETURN_TO_HOME;

        std::cout
            << "[FAULT] LOW_BATTERY"
            << std::endl;

        return;
    }

    if (signalFaultActive)
    {
        mode = FlightMode::SAFE_MODE;

        std::cout
            << "[FAULT] WEAK_SIGNAL"
            << std::endl;

        return;
    }

    mode = FlightMode::NORMAL;
}

FlightMode FaultManager::getMode() const
{
    return mode;
}

bool FaultManager::hasActiveFault() const
{
    return batteryFaultActive ||
           signalFaultActive ||
           temperatureFaultActive;
}

std::string FaultManager::getModeString() const
{
    switch (mode)
    {
        case FlightMode::NORMAL:
            return "NORMAL";

        case FlightMode::SAFE_MODE:
            return "SAFE_MODE";

        case FlightMode::RETURN_TO_HOME:
            return "RETURN_TO_HOME";
    }

    return "UNKNOWN";
}

}