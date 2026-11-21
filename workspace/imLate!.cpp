#include "imLate.h"

#include <algorithm>
#include <stdexcept>

namespace imlate {
namespace {

int priorityRank(Priority priority) {
    switch (priority) {
        case Priority::mustDo: return 0;
        case Priority::shrinkable: return 1;
        case Priority::movable: return 2;
    }
    return 3;
}

}

std::string priorityName(Priority priority) {
    switch (priority) {
        case Priority::mustDo: return "mustDo";
        case Priority::shrinkable: return "shrinkable";
        case Priority::movable: return "movable";
    }
    throw std::invalid_argument("Unknown task priority");
}

Schedule buildSchedule(std::time_t dayStart,
                       std::time_t dayEnd,
                       const std::vector<Task>& tasks,
                       std::time_t now) {
    const double dayLength = std::difftime(dayEnd, dayStart);
    if (dayLength <= 0 || dayLength > 48 * 60 * 60) {
        throw std::invalid_argument("Selected day boundaries are not valid");
    }

    if (dayEnd <= now) {
        return {0, {}, tasks};
    }
    std::time_t cursor = now > dayStart ? now : dayStart;

    const double secondsLeft = std::max(
        0.0, std::difftime(dayEnd, cursor));
    int remainingMinutes = static_cast<int>(secondsLeft / 60);
    Schedule result{remainingMinutes, {}, {}};

    std::vector<Task> orderedTasks = tasks;
    std::stable_sort(orderedTasks.begin(), orderedTasks.end(),
                     [](const Task& left, const Task& right) {
                         return priorityRank(left.priority) <
                                priorityRank(right.priority);
                     });

    for (const Task& task : orderedTasks) {
        if (task.durationMinutes <= 0) {
            throw std::invalid_argument("Task duration must be positive");
        }
        if (task.durationMinutes > remainingMinutes) {
            result.unscheduled.push_back(task);
            continue;
        }

        const std::time_t end =
            cursor + static_cast<std::time_t>(task.durationMinutes) * 60;
        result.scheduled.push_back({task, cursor, end});
        cursor = end;
        remainingMinutes -= task.durationMinutes;
    }
    return result;
}

}

