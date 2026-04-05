#pragma once

#ifndef DEVCONTAINER_JSON_POSTER_TASK_HPP
#define DEVCONTAINER_JSON_POSTER_TASK_HPP

#include "model.hpp"

/** Update the remote database with the temperatures. */
class PosterTask
{
    public:
        /**
         * @param model Model that contains the sensor readings.
         */
        static std::unique_ptr<PosterTask> start(Model& model);

        /**
         * @param model Model to read sensor readings from.
         */
        PosterTask(Model& model);
        ~PosterTask();

    private:
        PosterTask(const PosterTask&) = delete;
        PosterTask(PosterTask&&) = delete;
        PosterTask& operator=(const PosterTask&) = delete;
        PosterTask& operator=(PosterTask&&) = delete;

        /** Create the ESP-32 task. */
        static void poster_task_main(void* pvParameters);

    private:
        /** Model to update when there are new sensor readings. */
        Model& _model;
};


#endif //DEVCONTAINER_JSON_POSTER_TASK_HPP