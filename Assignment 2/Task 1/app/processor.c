/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Process samples and detect sensor alarms.
 * Date: 2026-11-06
 */

#include "sms_app.h"

#include <string.h>
#include <errno.h>

void *processor_thread(void *arg)
{
    (void)arg;

    float temperatures[ROLLING_WINDOW] = {0};
    float humidities[ROLLING_WINDOW] = {0};

    int sample_count = 0;
    int index = 0;

    float previous_temperature = 0.0f;
    bool has_previous = false;

    SensorData sample;

    while (buffer_pop(&sample)) {
        temperatures[index] = sample.temperature;
        humidities[index] = sample.humidity;

        index = (index + 1) % ROLLING_WINDOW;

        if (sample_count < ROLLING_WINDOW) {
            sample_count++;
        }

        float temperature_sum = 0.0f;
        float humidity_sum = 0.0f;

        for (int i = 0; i < sample_count; i++) {
            temperature_sum += temperatures[i];
            humidity_sum += humidities[i];
        }

        ProcessedData processed = {0};

processed.timestamp_ms = sample.timestamp_ms;
processed.temperature = sample.temperature;
processed.humidity = sample.humidity;
processed.avg_temperature = temperature_sum / sample_count;
processed.avg_humidity = humidity_sum / sample_count;

        printf("PROCESS: Temp=%.1f C, Humidity=%.1f%%, "
               "AvgTemp=%.1f C, AvgHumidity=%.1f%%\n",
               processed.temperature,
               processed.humidity,
               processed.avg_temperature,
               processed.avg_humidity);

        char alarm_text[128];

        if (sample.temperature > 40.0f) {
            snprintf(alarm_text, sizeof(alarm_text),
                     "HIGH TEMPERATURE: %.1f C",
                     sample.temperature);

            AlarmMessage alarm = {0};
            snprintf(alarm.message, sizeof(alarm.message),
                     "%s", alarm_text);

            if (mq_send(alarm_queue, (const char *)&alarm,
                        sizeof(alarm), 0) < 0) {
                perror("Processor: alarm mq_send failed");
            }
        }

        if (sample.humidity > 80.0f) {
            snprintf(alarm_text, sizeof(alarm_text),
                     "HIGH HUMIDITY: %.1f%%",
                     sample.humidity);

            AlarmMessage alarm = {0};
            snprintf(alarm.message, sizeof(alarm.message),
                     "%s", alarm_text);

            if (mq_send(alarm_queue, (const char *)&alarm,
                        sizeof(alarm), 0) < 0) {
                perror("Processor: alarm mq_send failed");
            }
        }

        if (has_previous &&
            (sample.temperature - previous_temperature > 5.0f ||
             previous_temperature - sample.temperature > 5.0f)) {

            snprintf(alarm_text, sizeof(alarm_text),
                     "TEMPERATURE CHANGE: %.1f -> %.1f C",
                     previous_temperature,
                     sample.temperature);

            AlarmMessage alarm = {0};
            snprintf(alarm.message, sizeof(alarm.message),
                     "%s", alarm_text);

            if (mq_send(alarm_queue, (const char *)&alarm,
                        sizeof(alarm), 0) < 0) {
                perror("Processor: alarm mq_send failed");
            }
        }

        previous_temperature = sample.temperature;
        has_previous = true;

        if (mq_send(log_queue, (const char *)&processed,
                    sizeof(processed), 0) < 0) {
            perror("Processor: log mq_send failed");
        }
    }

    printf("Processor: thread stopped\n");
    return NULL;
}
