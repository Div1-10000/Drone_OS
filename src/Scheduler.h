#ifndef DRONEOS_SCHEDULER_H
#define DRONEOS_SCHEDULER_H

#include <string>
#include <vector>

namespace DroneOS {

enum class Priority
{
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3
};

enum class TaskState
{
    READY,
    RUNNING,
    WAITING,
    COMPLETED
};

struct Task
{
    int id;
    std::string name;
    Priority priority;
    TaskState state;

    double cpuRequired;
    double memoryRequired;
    double powerRequired;
};

class Scheduler
{
private:
    std::vector<Task> tasks;

public:
    void initialize();

    void addTask(const Task& task);

    Task* getNextTask();

    void completeTask(Task& task);

    void setWaiting(Task& task);

    void resetCompletedTasks();

    const std::vector<Task>& getTasks() const;
};

const char* priorityToString(Priority priority);

const char* taskStateToString(TaskState state);

}

#endif