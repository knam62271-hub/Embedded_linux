/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Receive processed samples and write log file.
 * Date: 2026-11-06
 */

#include "sms_app.h"

#include <errno.h>

void *logger_thread(void *arg)
{
    (void)arg;

    FILE *log_file = fopen(LOG_PATH, "a");

    if (log_file == NULL) {
        perror("Logger: cannot open log file");
        running = 0;
        pthread_cond_broadcast(&buffer_not_empty);
        return NULL;
    }

    ProcessedData data;

    printf("Logger: writing to %s\n", LOG_PATH);

    while (1) {
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout) < 0) {
            perror("Logger: clock_gettime failed");
            break;
        }

        timeout.tv_sec += 1;

        ssize_t received = mq_timedreceive(
            log_queue,
            (char *)&data,
            sizeof(data),
            NULL,
            &timeout
        );

        if (received < 0) {
            if (errno == ETIMEDOUT || errno == EINTR) {
                struct mq_attr attr;

                if (processing_done &&
                    mq_getattr(log_queue, &attr) == 0 &&
                    attr.mq_curmsgs == 0) {
                    break;
                }

                continue;
            }

            perror("Logger: mq_timedreceive failed");
            break;
        }

        fprintf(log_file,
                "%llu,%.2f,%.2f,%.2f,%.2f\n",
                (unsigned long long)data.timestamp_ms,
                data.temperature,
                data.humidity,
                data.avg_temperature,
                data.avg_humidity);

        fflush(log_file);
    }

    fflush(log_file);
    fclose(log_file);

    printf("Logger: thread stopped\n");

    return NULL;
}
