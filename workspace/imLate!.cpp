#include "imLate.h"

#include <algorithm>
#include <stdexcept>

namespace imlate {
namespace {

// Assigns a numerical ranking to each priority.
int priorityRank(Priority priority) {
    switch (priority) {
        case Priority::mustDo: return 0;
        case Priority::shouldDo: return 1;
        case Priority::couldDo: return 2;
    }

    throw std::invalid_argument("Unknown task priority");
}

}

// Converts priority into a readable string.
std::string priorityName(Priority priority) {
    switch (priority) {
        case Priority::mustDo: return "mustDo";
        case Priority::shouldDo: return "shouldDo";
        case Priority::couldDo: return "couldDo";
    }

    throw std::invalid_argument("Unknown task priority");
}

Schedule buildSchedule(std::time_t dayStart,
                       std::time_t dayEnd,
                       const std::vector<Task>& tasks,
                       std::time_t now) {

    // Validate the selected day.
    const double dayLength = std::difftime(dayEnd, dayStart);

    if (dayLength <= 0 || dayLength > 48 * 60 * 60) {
        throw std::invalid_argument(
            "Selected day boundaries are not valid"
        );
    }

    // Start at the current time, or the beginning of the day.
    std::time_t cursor = now > dayStart ? now : dayStart;

    // Prepare the schedule.
    const double secondsLeft = std::max(
        0.0, std::difftime(dayEnd, cursor)
    );

    int availableMinutes = static_cast<int>(secondsLeft / 60);

    Schedule result{availableMinutes, {}, {}};

    // Remove completed tasks and validate unfinished tasks.
    std::vector<Task> orderedTasks;

    for (const Task& task : tasks) {

        if (task.completed) {
            continue;
        }

        if (task.durationMinutes <= 0) {
            throw std::invalid_argument(
                "Task duration must be positive"
            );
        }

        orderedTasks.push_back(task);
    }

    // Sort by earliest deadline, then by priority.
    std::stable_sort(
        orderedTasks.begin(),
        orderedTasks.end(),
        [](const Task& left, const Task& right) {

            if (left.deadline != right.deadline) {
                return left.deadline < right.deadline;
            }

            return priorityRank(left.priority) <
                   priorityRank(right.priority);
        }
    );

    // Schedule tasks in the sorted order.
    for (const Task& task : orderedTasks) {

        const std::time_t taskEnd =
            cursor + static_cast<std::time_t>(
                task.durationMinutes
            ) * 60;

        // Check whether the task can fit within the day
        // and finish before its deadline.
        if (taskEnd > dayEnd || taskEnd > task.deadline) {
            result.unscheduled.push_back(task);
            continue;
        }

        // Add the task to the schedule.
        result.scheduled.push_back({
            task,
            cursor,
            taskEnd
        });

        // Move the scheduling cursor forward.
        cursor = taskEnd;

        availableMinutes -= task.durationMinutes;
    }

    return result;
}

}