#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace imlate {

enum class Priority {
    mustDo,
    shrinkable,
    movable
};

struct Task {
    std::string title;
    int durationMinutes;
    Priority priority;
};

struct PlannedTask {
    Task task;
    std::time_t startTime;
    std::time_t endTime;
};

struct Schedule {
    int availableMinutes;
    std::vector<PlannedTask> scheduled;
    std::vector<Task> unscheduled;
};

Schedule buildSchedule(std::time_t dayStart,
                       std::time_t dayEnd,
                       const std::vector<Task>& tasks,
                       std::time_t now = std::time(nullptr));
std::string priorityName(Priority priority);

}
