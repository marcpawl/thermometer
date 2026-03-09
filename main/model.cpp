


extern "C" {
#include "esp_log.h"
    }

#include "model.hpp"

constexpr auto TAG = "model";

void ModelData::dump(const char* tag) const
{
    // Extract duration since epoch (boot)
    auto duration = sensor_readings._timestamp.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    ESP_LOGI(tag, "Time point: %lld ms", (long long)ms.count());

    for (auto const& reading : sensor_readings._readings)
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

    _publisher.publish('1');
}

Model::ModelSubscriber Model::subscribe()
{
    return _publisher.subscribe();
}
