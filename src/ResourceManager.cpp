#include "ResourceManager.h"

#include <algorithm>
#include <iostream>

namespace DroneOS {

ResourceManager::ResourceManager()
    : cpuCapacity(100.0),
      cpuAvailable(100.0),
      memoryCapacity(8192.0),
      memoryAvailable(8192.0),
      powerCapacity(600.0),
      powerAvailable(600.0),
      storageCapacity(100.0),
      storageAvailable(100.0)
{
}



void ResourceManager::initialize()
{
    std::cout
        << "Resource Manager started"
        << std::endl;

    std::cout
        << "[RESOURCE] CPU: "
        << cpuAvailable
        << "% available"
        << std::endl;

    std::cout
        << "[RESOURCE] MEMORY: "
        << memoryAvailable
        << " MB available"
        << std::endl;

    std::cout
        << "[RESOURCE] POWER: "
        << powerAvailable
        << "% available"
        << std::endl;
}

bool ResourceManager::allocate(
    double cpu,
    double memory,
    double power)
{
    if (cpu > cpuAvailable ||
        memory > memoryAvailable ||
        power > powerAvailable)
    {
        std::cout
            << "[RESOURCE] Allocation denied"
            << std::endl;

        return false;
    }

    cpuAvailable -= cpu;
    memoryAvailable -= memory;
    powerAvailable -= power;

    std::cout
        << "[RESOURCE] Allocated "
        << "CPU=" << cpu << "% "
        << "MEM=" << memory << "MB "
        << "POWER=" << power << "%"
        << std::endl;

    return true;
}

void ResourceManager::release(
    double cpu,
    double memory,
    double power)
{
    cpuAvailable =
        std::min(cpuCapacity, cpuAvailable + cpu);

    memoryAvailable =
        std::min(
            memoryCapacity,
            memoryAvailable + memory
        );

    powerAvailable =
        std::min(
            powerCapacity,
            powerAvailable + power
        );
}

double ResourceManager::getCpuAvailable() const
{
    return cpuAvailable;
}

double ResourceManager::getMemoryAvailable() const
{
    return memoryAvailable;
}

double ResourceManager::getPowerAvailable() const
{
    return powerAvailable;
}

double ResourceManager::getStorageAvailable() const
{
    return storageAvailable;
}

double ResourceManager::getCpuUsage() const
{
    return cpuCapacity - cpuAvailable;
}

double ResourceManager::getMemoryUsage() const
{
    return memoryCapacity - memoryAvailable;
}

double ResourceManager::getPowerUsage() const
{
    return powerCapacity - powerAvailable;
}

double ResourceManager::getStorageUsage() const
{
    return storageCapacity - storageAvailable;
}
void ResourceManager::updateDynamicUsage(
    double cpu,
    double memory,
    double power,
    double storage
)
{
    cpu = std::clamp(cpu, 0.0, cpuCapacity);
    memory = std::clamp(memory, 0.0, memoryCapacity);
    power = std::clamp(power, 0.0, powerCapacity);
    storage = std::clamp(storage, 0.0, storageCapacity);

    cpuAvailable =
        cpuCapacity - cpu;

    memoryAvailable =
        memoryCapacity - memory;

    powerAvailable =
        powerCapacity - power;

    storageAvailable =
        storageCapacity - storage;
}
}