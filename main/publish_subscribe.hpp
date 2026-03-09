
#pragma once
#ifndef DEVCONTAINER_JSON_PUBLISH_SUBSCRIBE_HPP
#define DEVCONTAINER_JSON_PUBLISH_SUBSCRIBE_HPP

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
}

#include <vector>
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>

template <size_t QueueDepth, typename T>
class Publisher;

template <typename T>
class Subscriber;

template <size_t QueueDepth, typename T>
class Publisher {
public:
    using data_t = T;
    using publisher_t = Publisher<QueueDepth, T>;
    using subscriber_t = Subscriber< publisher_t >;
    friend class Subscriber<Publisher<QueueDepth, T>>;

    using shared_ptr_t = std::shared_ptr<Publisher>;
    using weak_ptr_t = std::weak_ptr<Publisher>;
private:
    static constexpr const char* TAG = "Publisher";
    std::vector<QueueHandle_t> _queues;
    std::mutex _mutex;
    shared_ptr_t _myself;

    void unsubscribe(QueueHandle_t q) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::find(_queues.begin(), _queues.end(), q);
        if (it != _queues.end()) {
            _queues.erase(it);
            vQueueDelete(q);
        }
    }

public:
    Publisher()
        : _myself(this, [](auto){})
    {}

    // Delete copy/move to ensure identity
    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;
    Publisher(Publisher&&) = delete;
    Publisher& operator=(Publisher&&) = delete;

    ~Publisher() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_queues.empty()) {
            ESP_LOGE(TAG, "Shutting down. %zu subscribers.", _queues.size());
            abort();
        }
    }

    subscriber_t subscribe();

    void publish(const T& data);
};

template <typename PublisherType>
class Subscriber {
    friend PublisherType;

private:
    typename PublisherType::weak_ptr_t _parent;
    QueueHandle_t _queue;

    Subscriber(typename PublisherType::shared_ptr_t parent, QueueHandle_t q) : _parent(parent), _queue(q) {}

public:
    ~Subscriber() {
        if (_queue != nullptr) {
            typename PublisherType::shared_ptr_t shared_publisher = _parent.lock();
            if (shared_publisher != nullptr)
            {
                shared_publisher->unsubscribe(_queue);
            }
        }
    }

    // Move-only
    Subscriber(const Subscriber&) = delete;
    Subscriber(Subscriber&& other) noexcept = default;
    Subscriber& operator=(const Subscriber&) = delete;
    Subscriber& operator=(Subscriber&&) = default;

    /**
     * @return std::optional containing data if successful,
     */
    std::optional<typename PublisherType::data_t> wait() {
        typename PublisherType::data_t received;
        if (xQueueReceive(_queue, &received, portMAX_DELAY) == pdPASS) {
            return received;
        }
        return std::nullopt;
    }
};

template <size_t QueueDepth, typename T>
typename Publisher<QueueDepth, T>::subscriber_t Publisher<QueueDepth, T>::subscribe()
{
    QueueHandle_t q = xQueueCreate(QueueDepth, sizeof(T));
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queues.push_back(q);
    }
    return subscriber_t(_myself, q);
}

template <size_t QueueDepth, typename T>
void Publisher<QueueDepth, T>::publish(const T& data)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto q : _queues) {
        xQueueSend(q, &data, 0);
    }
}

#endif //DEVCONTAINER_JSON_PUBLISH_SUBSCRIBE_HPP