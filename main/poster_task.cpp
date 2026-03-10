
extern "C" {
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <esp_http_client.h>
}

#include "poster_task.hpp"
#include "rtc_clock.hpp"

constexpr auto TAG = "poster_task";
constexpr auto posting_interval = std::chrono::seconds(10);
constexpr auto stale_data_duration = std::chrono::minutes(1);
constexpr auto post_url =
    "https://script.google.com/macros/s/AKfycbyGTp_464C5KDiNwjZ-9KoXlV2uw3ZGOBEzWjvxPEG1nrQCJudjJfjCAhFTmAwcLkCU/exec";


PosterTask::PosterTask(Model& model)
    : _model(model)
{
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
        if (!first)
        {
            json << ", ";
        }
        json << "{";
        json << "\"temperature\": " << reading.temperature << ",";
        json << "\"address\": \"" << std::hex << reading.address << std::dec << "\"";
        json << "}";
        first = false;
    }
    json << "]";
    json << "}";

    return json.str();
}

static bool postToGoogleSheets(const std::string& payload)
{
    // Configuration for the HTTP client
    esp_http_client_config_t config = {};
    config.url = post_url;
    config.method = HTTP_METHOD_POST;
    // Increase timeout to 60 seconds to account for Google Script "cold starts"
    config.timeout_ms = 60000;
    config.skip_cert_common_name_check = true; // Use with caution in prod
    // Google requires following redirects (302)
    config.disable_auto_redirect = false;
    config.max_redirection_count = 10;
    config.crt_bundle_attach = esp_crt_bundle_attach; // Attach the trust bundle
    config.keep_alive_enable = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr)
    {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }

    // Set headers and body
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload.data(), payload.size());

    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    bool success = false;

    if (err == ESP_OK || err == ESP_ERR_HTTP_INCOMPLETE_DATA) {
        int status_code = esp_http_client_get_status_code(client);

        // Read the response body
        char buffer[32]; // Enough for "OK"
        int read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
        if (read_len >= 0) {
            buffer[read_len] = '\0'; // Null terminate
            ESP_LOGI(TAG, "HTTP Status: %d, Response: %s", status_code, buffer);

            // Check if the body contains our "OK"
            if (std::string_view(buffer).find("Success") != std::string_view::npos) {
                success = true;
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return success;
}

void PosterTask::poster_task_main(void* pvParameters)
{
    PosterTask* poster_task = static_cast<PosterTask*>(pvParameters);
    auto model_subscriber = poster_task->_model.subscribe();
    auto next_update = rtc_clock::now() - posting_interval;
    while (true)
    {
        model_subscriber.wait();
        auto now = rtc_clock::now();

        const ModelData model_data = poster_task->_model.read();
        if (model_data.sensor_readings._timestamp < next_update)
        {
            ESP_LOGI(TAG, "Throttling update. skipping");
        }
        else if (model_data.sensor_readings._timestamp < (now - stale_data_duration))
        {
            ESP_LOGI(TAG, "Readings are too old. skipping");
        }
        else
        {
            std::string json_data = to_json(model_data.sensor_readings);
            ESP_LOGI(TAG, "Posting data: %s", json_data.c_str());
            next_update = rtc_clock::now() + posting_interval;
            postToGoogleSheets(json_data);
        }
    }
}
