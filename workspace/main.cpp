#include <iostream>
#include <fstream>

#include <iostream>
#include <string>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
namespace std

string getCurrentTime() {

   auto now = chrono::system_clock::now();

   time_t currentTime = chrono::system_clock::to_time_t(now);

   tm* localTime = localtime(&currentTime);

   stringstream ss;

   ss << put_time(localTime, "%Y-%m-%d %H:%M:%S");

   return ss.str();
}

struct Task {

   int id;

   string task_name;

   string due_date;
   string due_time;

   string start_time;
   string estimated_end_time;

   int duration_minutes;

   int importance;

   string color;

   bool completed = false;
};



int main(){

   string startTime = getCurrentTime();

   cout << startTime;

   return 0;
}