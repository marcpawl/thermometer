
#pragma once
#ifndef DEVCONTAINER_JSON_MODEL_HPP
#define DEVCONTAINER_JSON_MODEL_HPP

#include "sequence_lock.hpp"
#include "sensor_readings.hpp"

struct ModelData
{
    SensorReadings sensor_readings;
};

class Model : public SequenceLock<ModelData>
{
private:
    using Lock = SequenceLock<ModelData>;

public:
    /** Update the model to use new sensor readings.
     * @parqm new_readings Readings that the model will be using.
     */
    void write(const SensorReadings& new_readings);

    /** Write the contents to the log. */
    void dump() const;
};

#endif //DEVCONTAINER_JSON_MODEL_HPP