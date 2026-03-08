


extern "C" {
#include "esp_log.h"
    }

#include "model.hpp"

constexpr auto TAG = "model";

void ModelData::dump(const char* tag) const
{
    for (auto const& reading : sensor_readings)
    {
        ESP_LOGI(tag, "temperature %" PRIX64 ": %.2fC", reading.address, reading.temperature);
    }
}

void Model::write(const SensorReadings& new_readings)
{
    SensorReadings* destination = &this->_data.sensor_readings;
    auto setter = [destination](SensorReadings const& readings)
    {
        *destination = readings;
    };
    SequenceLock<ModelData>::write<SensorReadings>(new_readings, setter);
    ESP_LOGI(TAG, "model updated sensor readings");
    _data.dump(TAG);
}
