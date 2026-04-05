
extern "C" {
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_rtc_time.h" // Required for RTC clock access
}
#include "rtc_clock.hpp"


rtc_clock::time_point rtc_clock::now() noexcept
{
    // Returns microseconds since boot, persisting through sleep
    // Note: Requires "RTC timer" to be enabled in menuconfig
    return time_point(duration(esp_rtc_get_time_us()));
}