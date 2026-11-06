/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Main application, thread management and shutdown.
 * Date: 2026-11-06
 */

#include "sms_app.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

volatile sig_atomic_t running = 1;
volatile sig_atomic_t processing_done = 0;

mqd_t alarm_queue = (mqd_t)-1;
mqd_t log_queue = (mqd_t)-1;

static void handle_signal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static int create_message_queues(void)
{
    struct mq_attr alarm_attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(AlarmMessage),
        .mq_curmsgs = 0
    };

    struct mq_attr log_attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(ProcessedData),
        .mq_curmsgs = 0
    };

    mq_unlink(ALARM_QUEUE_NAME);
    mq_unlink(LOG_QUEUE_NAME);

    alarm_queue = mq_open(
        ALARM_QUEUE_NAME,
        O_CREAT | O_RDWR,
        0644,
        &alarm_attr
    );

    if (alarm_queue == (mqd_t)-1) {
        perror("Cannot create alarm queue");
        return -1;
    }

    log_queue = mq_open(
        LOG_QUEUE_NAME,
        O_CREAT | O_RDWR,
        0644,
        &log_attr
    );

    if (log_queue == (mqd_t)-1) {
        perror("Cannot create log queue");
        mq_close(alarm_queue);
        mq_unlink(ALARM_QUEUE_NAME);
        alarm_queue = (mqd_t)-1;
        return -1;
    }

    return 0;
}

static void cleanup_message_queues(void)
{
    if (alarm_queue != (mqd_t)-1) {
        mq_close(alarm_queue);
        mq_unlink(ALARM_QUEUE_NAME);
    }

    if (log_queue != (mqd_t)-1) {
        mq_close(log_queue);
        mq_unlink(LOG_QUEUE_NAME);
    }
}

int main(void)
{
    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;

    struct sigaction signal_action;
    memset(&signal_action, 0, sizeof(signal_action));

    signal_action.sa_handler = handle_signal;
    sigemptyset(&signal_action.sa_mask);

    if (sigaction(SIGINT, &signal_action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    printf("=== Sensor Monitoring System ===\n");

    if (create_message_queues() < 0) {
        return EXIT_FAILURE;
    }

    if (pthread_create(&alert_tid, NULL, alert_thread, NULL) != 0) {
        fprintf(stderr, "Cannot create Alert Thread\n");
        cleanup_message_queues();
        return EXIT_FAILURE;
    }

    if (pthread_create(&logger_tid, NULL, logger_thread, NULL) != 0) {
        fprintf(stderr, "Cannot create Logger Thread\n");
        running = 0;
        processing_done = 1;
        pthread_join(alert_tid, NULL);
        cleanup_message_queues();
        return EXIT_FAILURE;
    }

    if (pthread_create(&processor_tid, NULL, processor_thread, NULL) != 0) {
        fprintf(stderr, "Cannot create Processor Thread\n");
        running = 0;
        processing_done = 1;
        pthread_join(alert_tid, NULL);
        pthread_join(logger_tid, NULL);
        cleanup_message_queues();
        return EXIT_FAILURE;
    }

    if (pthread_create(&sensor_tid, NULL, sensor_thread, NULL) != 0) {
        fprintf(stderr, "Cannot create Sensor Thread\n");

        running = 0;

        pthread_mutex_lock(&buffer_mutex);
        pthread_cond_broadcast(&buffer_not_empty);
        pthread_mutex_unlock(&buffer_mutex);

        pthread_join(processor_tid, NULL);

        processing_done = 1;

        pthread_join(alert_tid, NULL);
        pthread_join(logger_tid, NULL);

        cleanup_message_queues();
        return EXIT_FAILURE;
    }

    printf("All threads started. Press Ctrl+C to stop.\n");

    pthread_join(sensor_tid, NULL);

    running = 0;

    pthread_mutex_lock(&buffer_mutex);
    pthread_cond_broadcast(&buffer_not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    pthread_join(processor_tid, NULL);

    processing_done = 1;

    pthread_join(alert_tid, NULL);
    pthread_join(logger_tid, NULL);

    cleanup_message_queues();

    printf("Sensor Monitoring System stopped.\n");

    return EXIT_SUCCESS;
}
