
/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

extern "C" {
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ds18b20.h"
#include "onewire_bus.h"
}

#include "marcpawl_inplace_vector.hpp"
#include "sensor_task.hpp"

static const char* TAG = "sensor_task";


static Sensors discover_sensors(onewire_bus_handle_t bus)
{
    Sensors ds18b20s;
    onewire_device_iter_handle_t iter = nullptr;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    // create a 1-wire device iterator, which is used for device search
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "Device iterator created, start searching...");
    do
    {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK)
        {
            // found a new device, let's check if we can upgrade it to a DS18B20
            ds18b20_config_t ds_cfg = {};
            ds18b20_device_handle_t handle;
            if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &handle) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found a DS18B20 address: %" PRIX64 " %p", next_onewire_device.address, handle);
                ds18b20s.emplace_back(handle, next_onewire_device.address);
            }
            else
            {
                ESP_LOGI(TAG, "Found an unknown device, address: %" PRIX64, next_onewire_device.address);
            }
        }
    }
    while (search_result != ESP_ERR_NOT_FOUND);
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    ESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", ds18b20s.size());

    // set resolution for all DS18B20s
    for (auto const& sensor : ds18b20s)
    {
        // set resolution
        ESP_ERROR_CHECK(ds18b20_set_resolution(sensor.handle, DS18B20_RESOLUTION_12B));
    }

    return ds18b20s;
}

SensorReadings SensorTask::read_sensors()
{
    for (auto const& sensor : ds18b20s)
    {
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(sensor.handle));
    }
    vTaskDelay(pdMS_TO_TICKS(750));

    SensorReadings readings;
    for (auto const& sensor : ds18b20s)
    {
        float temperature;
        ESP_ERROR_CHECK(ds18b20_get_temperature(sensor.handle, &temperature));
        readings.emplace_back(temperature, sensor.address);
    }
    return readings;
}

void SensorTask::sensor_task_main(void* pvParameters)
{
    SensorTask* sensor_task = static_cast<SensorTask*>(pvParameters);
    while (true)
    {
        std::byte update_request;
        auto request_wanted = xQueueReceive(sensor_task->update_queue, &update_request, portMAX_DELAY);
        if (pdTRUE != request_wanted)
        {
            ESP_LOGW(TAG, "no update request received");
        } else {
            SensorReadings readings = sensor_task->read_sensors();
            sensor_task->_model.write(readings);
        }
    }
}

SensorTask::SensorTask(Model& model)
    :_model(model)
{
    update_queue = xQueueCreate(10, sizeof(std::byte));;

    // install a new 1-wire bus
    onewire_bus_handle_t bus;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = sensor_bus_gpio,
        .flags = {
            .en_pull_up = true, // enable the internal pull-up resistor in case the external device didn't have one
        }
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1 byte ROM command + 8 byte ROM number + 1 byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));
    ESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", sensor_bus_gpio);

    ds18b20s = discover_sensors(bus);
}

SensorTask::~SensorTask()
{
    ESP_LOGE(TAG, "task should never terminated");
    std::terminate();
}

void SensorTask::update()
{
    // Any value is used to indicate we want an update;
    std::byte update_request{1};
    xQueueSend(update_queue, &update_request, portMAX_DELAY);
}

std::unique_ptr<SensorTask> SensorTask::start(Model& model)
{
    auto sensor_task = std::make_unique<SensorTask>(model);
    xTaskCreate(&sensor_task_main, "sensor_task", 8192, sensor_task.get(), 5, nullptr);
    return sensor_task;
}
