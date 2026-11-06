/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Sensor acquisition thread.
 * Date: 2026-11-06
 */

#include "sms_app.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define SMS_SET_INTERVAL _IOW('S', 1, unsigned int)

void *sensor_thread(void *arg)
{
    (void)arg;

#ifdef HOST_SIMULATION

    /*
     * Host simulation:
     * Generate sensor data without requiring the ARM kernel driver.
     */
    struct timespec now;
    struct timespec delay = {
        .tv_sec = DEFAULT_INTERVAL_MS / 1000,
        .tv_nsec = (DEFAULT_INTERVAL_MS % 1000) * 1000000L
    };

    uint32_t random_state = 123456789U;

    clock_gettime(CLOCK_MONOTONIC, &now);
    random_state ^= (uint32_t)now.tv_nsec;

    printf("Sensor: HOST SIMULATION, interval %d ms\n",
           DEFAULT_INTERVAL_MS);

    while (running) {
        SensorData sample;

        clock_gettime(CLOCK_MONOTONIC, &now);

        sample.timestamp_ms =
            (uint64_t)now.tv_sec * 1000U +
            (uint64_t)now.tv_nsec / 1000000U;

        /* Simple local pseudo-random generator. */
        random_state ^= random_state << 13;
        random_state ^= random_state >> 17;
        random_state ^= random_state << 5;

        sample.temperature = 15.0f + (float)(random_state % 31U);

        random_state ^= random_state << 13;
        random_state ^= random_state >> 17;
        random_state ^= random_state << 5;

        sample.humidity = 30.0f + (float)(random_state % 61U);

        if (buffer_push(&sample) != 0) {
            fprintf(stderr,
                    "WARNING: Sensor buffer full, sample dropped\n");
        }

        delay.tv_sec = DEFAULT_INTERVAL_MS / 1000;
        delay.tv_nsec = (DEFAULT_INTERVAL_MS % 1000) * 1000000L;

        while (nanosleep(&delay, &delay) < 0 &&
               errno == EINTR && running) {
            /* Continue sleeping for the remaining time. */
        }
    }

    printf("Sensor: thread stopped\n");
    return NULL;

#else

    /* ARM mode: read sensor data from the kernel driver. */
    int fd;
    char line[128];

    fd = open(DEVICE_PATH, O_RDONLY);

    if (fd < 0) {
        perror("Sensor: cannot open device");
        running = 0;
        pthread_cond_broadcast(&buffer_not_empty);
        return NULL;
    }

    unsigned int interval = DEFAULT_INTERVAL_MS;

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        perror("Sensor: ioctl SMS_SET_INTERVAL failed");
        close(fd);
        running = 0;
        pthread_cond_broadcast(&buffer_not_empty);
        return NULL;
    }

    printf("Sensor: reading interval set to %u ms\n", interval);

    while (running) {
        ssize_t bytes_read = read(fd, line, sizeof(line) - 1);

        if (bytes_read < 0) {
            if (errno == EINTR && !running) {
                break;
            }

            perror("Sensor: read failed");
            break;
        }

        if (bytes_read == 0) {
            continue;
        }

        line[bytes_read] = '\0';

        SensorData sample;
        unsigned long long timestamp;

        if (sscanf(line, "%llu,%f,%f",
                   &timestamp,
                   &sample.temperature,
                   &sample.humidity) != 3) {
            fprintf(stderr, "Sensor: invalid CSV: %s\n", line);
            continue;
        }

        sample.timestamp_ms = (uint64_t)timestamp;

        if (buffer_push(&sample) != 0) {
            fprintf(stderr,
                    "WARNING: Sensor buffer full, sample dropped\n");
        }
    }

    close(fd);
    printf("Sensor: thread stopped\n");

    return NULL;

#endif
}
