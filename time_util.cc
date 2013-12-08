#include "time_util.h"

Clock::time_point GetTime() {
  return Clock::now();
}

double GetDuration(const Clock::time_point& start_time,
                   const Clock::time_point& end_time) {
  return duration_cast<milliseconds>(end_time - start_time).count() / 1000.0;
}
