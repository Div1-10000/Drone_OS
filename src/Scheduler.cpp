#include "Scheduler.h"

#include <iostream>

namespace DroneOS {

void Scheduler::initialize()
{
    std::cout << "Scheduler started" << std::endl;
}

void Scheduler::addTask(const Task& task)
{
    tasks.push_back(task);

    std::cout
        << "[SCHEDULER] Task queued: "
        << task.name
        << " ["
        << priorityToString(task.priority)
        << "]"
        << std::endl;
}

Task* Scheduler::getNextTask()
{
    Task* bestTask = nullptr;

    for (auto& task : tasks)
    {
        if (task.state != TaskState::READY)
            continue;

        if (bestTask == nullptr ||
            static_cast<int>(task.priority) <
            static_cast<int>(bestTask->priority))
        {
            bestTask = &task;
        }
    }

    return bestTask;
}

void Scheduler::completeTask(Task& task)
{
    task.state = TaskState::COMPLETED;

    std::cout
        << "[SCHEDULER] Task completed: "
        << task.name
        << std::endl;
}

void Scheduler::setWaiting(Task& task)
{
    task.state = TaskState::WAITING;

    std::cout
        << "[SCHEDULER] Task waiting: "
        << task.name
        << std::endl;
}

void Scheduler::resetCompletedTasks()
{
    for (auto& task : tasks)
    {
        if (task.state == TaskState::COMPLETED)
        {
            task.state = TaskState::READY;
        }
    }
}

const std::vector<Task>& Scheduler::getTasks() const
{
    return tasks;
}

const char* priorityToString(Priority priority)
{
    switch (priority)
    {
        case Priority::CRITICAL:
            return "CRITICAL";

        case Priority::HIGH:
            return "HIGH";

        case Priority::MEDIUM:
            return "MEDIUM";

        case Priority::LOW:
            return "LOW";
    }

    return "UNKNOWN";
}

const char* taskStateToString(TaskState state)
{
    switch (state)
    {
        case TaskState::READY:
            return "READY";

        case TaskState::RUNNING:
            return "RUNNING";

        case TaskState::WAITING:
            return "WAITING";

        case TaskState::COMPLETED:
            return "COMPLETED";
    }

    return "UNKNOWN";
}

}