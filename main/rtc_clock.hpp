
#pragma once

#ifndef DEVCONTAINER_JSON_RTC_CLOCK_HPP
#define DEVCONTAINER_JSON_RTC_CLOCK_HPP


#include <chrono>

struct rtc_clock {
    using rep = int64_t;
    using period = std::micro;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<rtc_clock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept;
};

#endif //DEVCONTAINER_JSON_RTC_CLOCK_HPP