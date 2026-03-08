


extern "C" {
#include "esp_log.h"
    }

#include "model.hpp"

constexpr auto TAG = "model";

void Model::write(const SensorReadings& new_readings)
{
    SensorReadings* destination = &this->_data.sensor_readings;
    auto setter = [destination](SensorReadings const& readings)
    {
        *destination = readings;
    };
    SequenceLock<ModelData>::write<SensorReadings>(new_readings, setter);
    dump();
}

void Model::dump() const
{
    for (auto const& reading : _data.sensor_readings)
    {
        ESP_LOGI(TAG, "temperature %" PRIX64 ": %.2fC", reading.address, reading.temperature);
    }
}