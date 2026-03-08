//
// Created by marcpawl on 2026-03-08.
//

#pragma once

#ifndef DEVCONTAINER_JSON_SENSOR_READINGS_HPP
#define DEVCONTAINER_JSON_SENSOR_READINGS_HPP

extern "C" {
#include "freertos/FreeRTOS.h"
#include "onewire_bus.h"
}
#include "marcpawl_inplace_vector.hpp"

static constexpr auto max_sensors = 2;

struct SensorReading
{
    float temperature;
    onewire_device_address_t address;
};
static_assert(std::is_trivial_v<SensorReading>);

using SensorReadings = marcpawl::inplace_vector<SensorReading, max_sensors>;
static_assert(std::is_trivially_copyable_v<SensorReadings>);

#endif //DEVCONTAINER_JSON_SENSOR_READINGS_HPPstruct SesnorReading
