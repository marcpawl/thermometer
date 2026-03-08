//
// Created by marcpawl on 2026-03-07.
//

#pragma once
#ifndef DEVCONTAINER_JSON_SENSOR_TASK_HPP
#define DEVCONTAINER_JSON_SENSOR_TASK_HPP

extern "C" {
#include "ds18b20.h"
}

#include "marcpawl_inplace_vector.hpp"

static constexpr auto sensor_bus_gpio = 4;
static constexpr auto max_sensors = 2;

/** Sensor readings of this value or larger are invalid. */
constexpr float max_temperature = 85.0;

struct Sensor
{
    ds18b20_device_handle_t handle;
    onewire_device_address_t address;
};

using Sensors = marcpawl::inplace_vector<Sensor, max_sensors>;

struct SesnorReading
{
    float temperature;
    onewire_device_address_t address;
};

using SensorReadings = marcpawl::inplace_vector<SesnorReading, max_sensors>;

class SensorTask
{
public:
    static std::unique_ptr<SensorTask> start();
    SensorTask();
    ~SensorTask();

private:
    SensorTask(const SensorTask&) = delete;
    SensorTask(SensorTask&&) = delete;
    SensorTask& operator=(const SensorTask&) = delete;
    SensorTask& operator=(SensorTask&&) = delete;

    SensorReadings read_sensors();

    /** Create the ESP-32 task. */
    static void sensor_task_main(void* pvParameters);

public:
    /** Send a request for new sensor readings. */
    void update();

private:
    Sensors ds18b20s;

    /** Queue to read requests from. */
    QueueHandle_t update_queue;
};

#endif //DEVCONTAINER_JSON_SENSOR_TASK_HPP
