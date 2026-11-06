/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Thread-safe circular buffer implementation.
 * Date: 2026-11-06
 */

#include "sms_app.h"

CircularBuffer sensor_buffer = {0};

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;

/*
 * Push a sensor sample into the circular buffer.
 * Return 0 on success, -1 if the buffer is full.
 */
int buffer_push(const SensorData *data)
{
    if (data == NULL) {
        return -1;
    }

    pthread_mutex_lock(&buffer_mutex);

    if (sensor_buffer.count == BUFFER_SIZE) {
        pthread_mutex_unlock(&buffer_mutex);
        return -1;
    }

    sensor_buffer.data[sensor_buffer.tail] = *data;

    sensor_buffer.tail =
        (sensor_buffer.tail + 1) % BUFFER_SIZE;

    sensor_buffer.count++;

    pthread_cond_signal(&buffer_not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    return 0;
}

/*
 * Pop a sensor sample from the circular buffer.
 * Wait while empty and application is running.
 * Return 1 on success, 0 when shutting down.
 */
int buffer_pop(SensorData *data)
{
    if (data == NULL) {
        return 0;
    }

    pthread_mutex_lock(&buffer_mutex);

    while (sensor_buffer.count == 0 && running) {
        pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
    }

    if (sensor_buffer.count == 0 && !running) {
        pthread_mutex_unlock(&buffer_mutex);
        return 0;
    }

    *data = sensor_buffer.data[sensor_buffer.head];

    sensor_buffer.head =
        (sensor_buffer.head + 1) % BUFFER_SIZE;

    sensor_buffer.count--;

    pthread_cond_signal(&buffer_not_full);
    pthread_mutex_unlock(&buffer_mutex);

    return 1;
}
