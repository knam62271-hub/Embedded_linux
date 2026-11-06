/*
 * Assignment 02 - Sensor Monitoring System
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Shared definitions for SMS application threads.
 * Date: 2026-11-06
 */

#ifndef SMS_APP_H
#define SMS_APP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <time.h>

#define DEVICE_PATH "/dev/sms_sensor"
#define LOG_PATH "/tmp/sms.log"

#define ALARM_QUEUE_NAME "/alarm_queue"
#define LOG_QUEUE_NAME   "/log_queue"

#define BUFFER_SIZE 32
#define ROLLING_WINDOW 10

#define DEFAULT_INTERVAL_MS 500
#define SENSOR_INTERVAL_MS 500

/* Raw sensor sample */
typedef struct {
    uint64_t timestamp_ms;
    float temperature;
    float humidity;
} SensorData;

/* Processed sensor sample */
typedef struct {
    uint64_t timestamp_ms;
    float temperature;
    float humidity;
    float avg_temperature;
    float avg_humidity;
} ProcessedData;

/* Alarm message */
typedef struct {
    char message[128];
} AlarmMessage;

/* Circular buffer */
typedef struct {
    SensorData data[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} CircularBuffer;

/* Shared variables */
extern CircularBuffer sensor_buffer;
extern pthread_mutex_t buffer_mutex;
extern pthread_cond_t buffer_not_empty;
extern pthread_cond_t buffer_not_full;

extern volatile sig_atomic_t running;
extern volatile sig_atomic_t processing_done;

extern mqd_t alarm_queue;
extern mqd_t log_queue;

/* Thread functions */
void *sensor_thread(void *arg);
void *processor_thread(void *arg);
void *alert_thread(void *arg);
void *logger_thread(void *arg);

/* Circular buffer functions */
int buffer_push(const SensorData *data);
int buffer_pop(SensorData *data);

#endif
