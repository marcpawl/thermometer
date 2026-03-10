
extern "C" {
#include "esp_log.h"
}
#include "poster_task.hpp"
#include "rtc_clock.hpp"

constexpr auto TAG = "poster_task";

PosterTask::PosterTask(Model& model)
    : _model(model){
}

PosterTask::~PosterTask()
{
    ESP_LOGE(TAG, "task should never terminated");
    std::terminate();
}

std::unique_ptr<PosterTask> PosterTask::start(Model& model)
{
    auto poster_task = std::make_unique<PosterTask>(model);
    xTaskCreate(&poster_task_main, "poster_task", 8192, poster_task.get(), 5, nullptr);
    return poster_task;
}

// TODO stop using dynamic memory
static std::string to_json(SensorReadings const& readings)
{
    std::stringstream json;
    json << "{";
    json << "\"sensor_readings\": ";
    json << "[";
    bool first = true;
    for (auto const& reading : readings._readings)
    {
        if (! first)
        {
            json << ", ";
        }
        json << "{";
        json << "\"temperature\": " << reading.temperature << ",";
        json << "\"address\": \"" << std::hex <<  reading.address << std::dec << "\"";
        json << "}";
        first = false;
    }
    json << "]";
    json << "}";

    return json.str();
}

void PosterTask::poster_task_main(void* pvParameters)
{
    PosterTask* poster_task = static_cast<PosterTask*>(pvParameters);
    auto model_subscriber = poster_task->_model.subscribe();
    auto next_update = rtc_clock::now() + std::chrono::seconds(10);
    while (true)
    {
        model_subscriber.wait();
        auto now = rtc_clock::now();

        const ModelData model_data = poster_task->_model.read();
        if (model_data.sensor_readings._timestamp < next_update)
        {
            ESP_LOGI(TAG, "Throttling update. skipping");
        } else if (model_data.sensor_readings._timestamp < (now - std::chrono::minutes(1))) {
            ESP_LOGI(TAG, "Readings are too old. skipping");
        }
        else
        {
            std::string json_data = to_json(model_data.sensor_readings);
            ESP_LOGI(TAG, "Posting data: %s", json_data.c_str());
            next_update = rtc_clock::now() + std::chrono::seconds(10);
        }
    }
}