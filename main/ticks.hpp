//
// Created by marcpawl on 2026-03-09.
//

#pragma once

#ifndef DEVCONTAINER_JSON_TICKS_HPP
#define DEVCONTAINER_JSON_TICKS_HPP

#include <chrono>
#include "freertos/FreeRTOS.h"

inline TickType_t to_ticks(std::chrono::milliseconds ms) {
    if (ms == std::chrono::milliseconds::max()) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(ms.count());
}

#endif //DEVCONTAINER_JSON_TICKS_HPP