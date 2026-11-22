// #include "imLate.h"

// #include <algorithm>
// #include <stdexcept>

// namespace imlate {
// namespace {

// int priorityRank(Priority priority) {
//     switch (priority) {
//         case Priority::mustDo: return 0;
//         case Priority::shouldDo: return 1;
//         case Priority::couldDo: return 2;
//     }
//     return 3;
// }

// }

// std::string priorityName(Priority priority) {
//     switch (priority) {
//         case Priority::mustDo: return "mustDo";
//         case Priority::shouldDo: return "shouldDo";
//         case Priority::couldDo: return "couldDo";
//     }
//     throw std::invalid_argument("Unknown task priority");
// }

// Schedule buildSchedule(std::time_t dayStart,
//                        std::time_t dayEnd,
//                        const std::vector<Task>& tasks,
//                        std::time_t now) {
//     const double dayLength = std::difftime(dayEnd, dayStart);
//     if (dayLength <= 0 || dayLength > 48 * 60 * 60) {
//         throw std::invalid_argument("Selected day boundaries are not valid");
//     }

//     if (dayEnd <= now) {
//         return {0, {}, tasks};
//     }
//     std::time_t cursor = now > dayStart ? now : dayStart;

//     const double secondsLeft = std::max(
//         0.0, std::difftime(dayEnd, cursor));
//     int remainingMinutes = static_cast<int>(secondsLeft / 60);
//     Schedule result{remainingMinutes, {}, {}};

//     std::vector<Task> orderedTasks = tasks;
//     std::stable_sort(orderedTasks.begin(), orderedTasks.end(),
//                      [](const Task& left, const Task& right) {
//                          return priorityRank(left.priority) <
//                                 priorityRank(right.priority);
//                      });

//     for (const Task& task : orderedTasks) {
//         if (task.durationMinutes <= 0) {
//             throw std::invalid_argument("Task duration must be positive");
//         }
//         if (task.durationMinutes > remainingMinutes) {
//             result.unscheduled.push_back(task);
//             continue;
//         }

//         const std::time_t end =
//             cursor + static_cast<std::time_t>(task.durationMinutes) * 60;
//         result.scheduled.push_back({task, cursor, end});
//         cursor = end;
//         remainingMinutes -= task.durationMinutes;
//     }
//     return result;
// }

// }

#include "imLate.h"

#include <algorithm>
#include <stdexcept>
#include <limits>

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

// Finds a continuous available time slot for a task.
bool findAvailableSlot(
    std::time_t cursor,
    std::time_t latestEnd,
    int durationMinutes,
    const std::vector<UnavailablePeriod>& unavailable,
    std::time_t& start,
    std::time_t& end
) {
    const std::time_t duration =
        static_cast<std::time_t>(durationMinutes) * 60;

    std::time_t candidate = cursor;

    for (const auto& period : unavailable) {

        // The task fits before this unavailable period.
        if (candidate <= period.startTime &&
            duration <= period.startTime - candidate) {

            start = candidate;
            end = candidate + duration;
            return end <= latestEnd;
        }

        // Move past an unavailable period if necessary.
        if (candidate < period.endTime) {
            candidate = period.endTime;
        }

        // There is no longer enough time.
        if (candidate > latestEnd ||
            duration > latestEnd - candidate) {
            return false;
        }
    }

    // Check the time remaining after the last unavailable period.
    if (candidate <= latestEnd &&
        duration <= latestEnd - candidate) {

        start = candidate;
        end = candidate + duration;
        return true;
    }

    return false;
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

// Main daily scheduling algorithm.
Schedule buildSchedule(
    std::time_t dayStart,
    std::time_t dayEnd,
    const std::vector<Task>& tasks,
    std::time_t now,
    const std::vector<UnavailablePeriod>& unavailable
) {

    // Validate selected day.
    const double dayLength = std::difftime(dayEnd, dayStart);

    if (dayLength <= 0 || dayLength > 48 * 60 * 60) {
        throw std::invalid_argument("Selected day boundaries are not valid");
    }

    // If the selected day has already ended.
    if (dayEnd <= now) {
        Schedule result{0, {}, {}};

        for (const Task& task : tasks) {
            if (!task.completed) {
                result.unscheduled.push_back(task);
            }
        }

        return result;
    }

    // Determine when scheduling begins.
    std::time_t cursor = now > dayStart ? now : dayStart;

    // Remove completed tasks and validate remaining tasks.
    std::vector<Task> orderedTasks;

    for (const Task& task : tasks) {

        if (task.completed) {
            continue;
        }

        if (task.durationMinutes <= 0) {
            throw std::invalid_argument("Task duration must be positive");
        }

        if (task.deadline <= 0) {
            throw std::invalid_argument("Task deadline is invalid");
        }

        orderedTasks.push_back(task);
    }

    // Validate and prepare unavailable periods.
    std::vector<UnavailablePeriod> busyPeriods;

    for (const auto& period : unavailable) {

        if (period.endTime <= period.startTime) {
            throw std::invalid_argument("Unavailable period is invalid");
        }

        // Ignore periods outside the remaining scheduling window.
        std::time_t start = std::max(period.startTime, cursor);
        std::time_t end = std::min(period.endTime, dayEnd);

        if (start < end) {
            busyPeriods.push_back({start, end});
        }
    }

    // Sort unavailable periods by starting time.
    std::sort(
        busyPeriods.begin(),
        busyPeriods.end(),
        [](const UnavailablePeriod& a, const UnavailablePeriod& b) {
            return a.startTime < b.startTime;
        }
    );

    // Merge overlapping unavailable periods.
    std::vector<UnavailablePeriod> mergedPeriods;

    for (const auto& period : busyPeriods) {

        if (mergedPeriods.empty() ||
            period.startTime > mergedPeriods.back().endTime) {

            mergedPeriods.push_back(period);

        } else {

            mergedPeriods.back().endTime =
                std::max(
                    mergedPeriods.back().endTime,
                    period.endTime
                );
        }
    }

    busyPeriods = mergedPeriods;

    // Calculate total available minutes.
    double availableSeconds = std::difftime(dayEnd, cursor);

    for (const auto& period : busyPeriods) {
        availableSeconds -=
            std::difftime(period.endTime, period.startTime);
    }

    int availableMinutes =
        static_cast<int>(availableSeconds / 60);

    Schedule result{availableMinutes, {}, {}};

    // Higher-priority tasks are scheduled before lower-priority tasks.
    // Deadlines order tasks within the same priority.
    std::stable_sort(
        orderedTasks.begin(),
        orderedTasks.end(),
        [](const Task& a, const Task& b) {
            if (priorityRank(a.priority) != priorityRank(b.priority)) {
                return priorityRank(a.priority) < priorityRank(b.priority);
            }

            return a.deadline < b.deadline;
        }
    );

    // Attempt to schedule every task.
    for (const Task& task : orderedTasks) {

        // A task cannot finish after the end of the selected day.
        // It also cannot finish after its own deadline.
        const std::time_t latestEnd =
            std::min(task.deadline, dayEnd);

        std::time_t start;
        std::time_t end;

        bool fits = findAvailableSlot(
            cursor,
            latestEnd,
            task.durationMinutes,
            busyPeriods,
            start,
            end
        );

        if (!fits) {
            result.unscheduled.push_back(task);
            continue;
        }

        // Add successfully scheduled task.
        result.scheduled.push_back({task, start, end});

        // Move scheduling cursor forward.
        cursor = end;
    }

    return result;
}

}