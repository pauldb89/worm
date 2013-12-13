#pragma once

#include <chrono>

using namespace std;
using namespace chrono;

typedef high_resolution_clock Clock;

Clock::time_point GetTime();

double GetDuration(const Clock::time_point& start_time,
                   const Clock::time_point& end_time);
