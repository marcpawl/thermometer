
#pragma once
#ifndef DEVCONTAINER_JSON_MODEL_HPP
#define DEVCONTAINER_JSON_MODEL_HPP

#include "publish_subscribe.hpp"
#include "sequence_lock.hpp"
#include "sensor_readings.hpp"

struct ModelData
{
    SensorReadings sensor_readings;

    /** Write the contents to the log.
     *  @param tag Log to write to.
     */
    void dump(char const * tag) const;
};

class Model : public SequenceLock<ModelData>
{
private:
    using Lock = SequenceLock<ModelData>;
    using ModelPublisher = Publisher<1, char>;
public:
    using ModelSubscriber = ModelPublisher::subscriber_t;

private:
    ModelPublisher _publisher;

public:
    /** Update the model to use new sensor readings.
     * @parqm new_readings Readings that the model will be using.
     */
    void write(const SensorReadings& new_readings);

    ModelSubscriber subscribe();
};

#endif //DEVCONTAINER_JSON_MODEL_HPP