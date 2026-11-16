#ifndef DRONEOS_RESOURCE_MANAGER_H
#define DRONEOS_RESOURCE_MANAGER_H

namespace DroneOS {

class ResourceManager
{
private:
    double cpuCapacity;
    double cpuAvailable;

    double memoryCapacity;
    double memoryAvailable;

    double powerCapacity;
    double powerAvailable;

    double storageCapacity;
    double storageAvailable;

public:
    ResourceManager();

    void initialize();

    bool allocate(
        double cpu,
        double memory,
        double power
    );

    void release(
        double cpu,
        double memory,
        double power
    );

    double getCpuAvailable() const;
    double getMemoryAvailable() const;
    double getPowerAvailable() const;
    double getStorageAvailable() const;

    double getCpuUsage() const;
    double getMemoryUsage() const;
    double getPowerUsage() const;
    double getStorageUsage() const;
    
    void updateDynamicUsage(
        double cpu,
        double memory,
        double power,
        double storage
    );
};

}

#endif