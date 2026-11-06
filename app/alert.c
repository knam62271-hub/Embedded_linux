/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Receive and display sensor alarms.
 * Date: 2026-11-06
 */

#include "sms_app.h"

#include <errno.h>
#include <string.h>

void *alert_thread(void *arg)
{
    (void)arg;

    AlarmMessage alarm;
    printf("Alert: thread started\n");

    while (1) {
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout) < 0) {
            perror("Alert: clock_gettime failed");
            break;
        }

        timeout.tv_sec += 1;

        ssize_t received = mq_timedreceive(
            alarm_queue,
            (char *)&alarm,
            sizeof(alarm),
            NULL,
            &timeout
        );

        if (received < 0) {
            if (errno == ETIMEDOUT || errno == EINTR) {
                struct mq_attr attr;

                if (processing_done &&
                    mq_getattr(alarm_queue, &attr) == 0 &&
                    attr.mq_curmsgs == 0) {
                    break;
                }

                continue;
            }

            perror("Alert: mq_timedreceive failed");
            break;
        }

        alarm.message[sizeof(alarm.message) - 1] = '\0';

        printf("\n========== SENSOR ALARM ==========\n");
        printf("ALERT: %s\n", alarm.message);
        printf("==================================\n");
    }

    printf("Alert: thread stopped\n");
    return NULL;
}
