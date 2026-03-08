
extern "C" {
#include "esp_log.h"
}
#include "poster_task.hpp"

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

void PosterTask::poster_task_main(void* pvParameters)
{
    PosterTask* poster_task = static_cast<PosterTask*>(pvParameters);
    while (true)
    {
        const ModelData model_data = poster_task->_model.read();
        ESP_LOGI(TAG, "dumping model data");
        model_data.dump(TAG);
        vTaskDelay(10000*3 / portTICK_PERIOD_MS);
    }
}