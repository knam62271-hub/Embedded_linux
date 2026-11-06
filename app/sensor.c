
/*
 * Assignment 02 - Sensor Monitoring System
 * Task 4: Real-time Scheduling and Jitter Measurement
 */

#include "sms_app.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <limits.h>

#define SMS_SET_INTERVAL _IOW('S', 1, unsigned int)

#define JITTER_MAX_SAMPLES 10000
#define JITTER_REQUIRED 1000

static int64_t jitter_samples[JITTER_MAX_SAMPLES];
static unsigned int jitter_count = 0;

static int compare_int64(const void *a, const void *b)
{
    int64_t x = *(const int64_t *)a;
    int64_t y = *(const int64_t *)b;

    return (x > y) - (x < y);
}

static int64_t timespec_to_ns(struct timespec t)
{
    return (int64_t)t.tv_sec * 1000000000LL + t.tv_nsec;
}

static void add_jitter(int64_t jitter)
{
    if (jitter_count < JITTER_MAX_SAMPLES) {
        jitter_samples[jitter_count++] = jitter;
    }
}

static void print_jitter_report(void)
{
    if (jitter_count == 0) {
        printf("No jitter samples collected.\n");
        return;
    }

    int64_t sorted[JITTER_MAX_SAMPLES];
    int64_t sum = 0;
    int64_t min;
    int64_t max;

    memcpy(sorted, jitter_samples,
           jitter_count * sizeof(int64_t));

    qsort(sorted, jitter_count, sizeof(int64_t), compare_int64);

    min = sorted[0];
    max = sorted[jitter_count - 1];

    for (unsigned int i = 0; i < jitter_count; i++) {
        sum += jitter_samples[i];
    }

    unsigned int p99_index =
        (unsigned int)((jitter_count * 99ULL + 99ULL) / 100ULL) - 1;

    printf("\n===== JITTER REPORT =====\n");
    printf("Samples: %u\n", jitter_count);
    printf("Minimum: %.3f us\n", min / 1000.0);
    printf("Average: %.3f us\n",
           (double)sum / jitter_count / 1000.0);
    printf("Maximum: %.3f us\n", max / 1000.0);
    printf("P99: %.3f us\n", sorted[p99_index] / 1000.0);

    unsigned int bins[5] = {0};

    for (unsigned int i = 0; i < jitter_count; i++) {
        int64_t value = jitter_samples[i];

        if (value <= 50000)
            bins[0]++;
        else if (value <= 100000)
            bins[1]++;
        else if (value <= 500000)
            bins[2]++;
        else if (value <= 1000000)
            bins[3]++;
        else
            bins[4]++;
    }

    printf("\nJitter Histogram:\n");
    printf("  0 - 50 us:       %u\n", bins[0]);
    printf("  50 - 100 us:     %u\n", bins[1]);
    printf("  100 - 500 us:    %u\n", bins[2]);
    printf("  500 - 1000 us:   %u\n", bins[3]);
    printf("  Over 1000 us:    %u\n", bins[4]);

    if (jitter_count < JITTER_REQUIRED) {
        printf("WARNING: Need at least %d samples.\n",
               JITTER_REQUIRED);
    }

    printf("=========================\n");
}

static unsigned int get_interval(void)
{
    const char *value = getenv("SMS_INTERVAL_MS");

    if (value == NULL)
        return DEFAULT_INTERVAL_MS;

    char *end;
    unsigned long interval = strtoul(value, &end, 10);

    if (*value == '\0' || *end != '\0' ||
        interval == 0 || interval > 60000) {
        fprintf(stderr, "Invalid SMS_INTERVAL_MS, using default.\n");
        return DEFAULT_INTERVAL_MS;
    }

#ifndef HOST_SIMULATION
    if (interval < 100) {
        fprintf(stderr,
                "ARM driver requires interval >= 100 ms.\n");
        return DEFAULT_INTERVAL_MS;
    }
#endif

    return (unsigned int)interval;
}

static void configure_scheduler(void)
{
    const char *mode = getenv("SMS_SCHED");

    struct sched_param param;
    memset(&param, 0, sizeof(param));

    if (mode != NULL && strcmp(mode, "fifo") == 0) {
        param.sched_priority = 50;

        int ret = pthread_setschedparam(
            pthread_self(), SCHED_FIFO, &param);

        if (ret == 0) {
            printf("Sensor scheduler: SCHED_FIFO, priority 50\n");
        } else {
            fprintf(stderr,
                    "Cannot enable SCHED_FIFO: %s\n",
                    strerror(ret));
            printf("Sensor scheduler: using SCHED_OTHER\n");
        }
    } else {
        param.sched_priority = 0;

        int ret = pthread_setschedparam(
            pthread_self(), SCHED_OTHER, &param);

        if (ret != 0) {
            fprintf(stderr,
                    "Cannot set SCHED_OTHER: %s\n",
                    strerror(ret));
        }

        printf("Sensor scheduler: SCHED_OTHER\n");
    }
}

static int wait_until(struct timespec *deadline)
{
    int ret;

    do {
        ret = clock_nanosleep(
            CLOCK_MONOTONIC,
            TIMER_ABSTIME,
            deadline,
            NULL);
    } while (ret == EINTR && running);

    return ret;
}

static void advance_deadline(struct timespec *deadline,
                             unsigned int interval_ms)
{
    deadline->tv_nsec += (long)interval_ms * 1000000L;

    while (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec++;
        deadline->tv_nsec -= 1000000000L;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    /* Skip missed periods instead of producing burst samples. */
    while (deadline->tv_sec < now.tv_sec ||
          (deadline->tv_sec == now.tv_sec &&
           deadline->tv_nsec <= now.tv_nsec)) {

        deadline->tv_nsec += (long)interval_ms * 1000000L;

        while (deadline->tv_nsec >= 1000000000L) {
            deadline->tv_sec++;
            deadline->tv_nsec -= 1000000000L;
        }
    }
}

void *sensor_thread(void *arg)
{
    (void)arg;

    configure_scheduler();

    unsigned int interval = get_interval();

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);

    deadline.tv_nsec += (long)interval * 1000000L;

    while (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    printf("Sensor interval: %u ms\n", interval);

#ifdef HOST_SIMULATION

    uint32_t random_state = 123456789U;

    printf("Sensor: HOST SIMULATION\n");

    while (running) {
        SensorData sample;

        int ret = wait_until(&deadline);

        if (ret != 0) {
            if (running)
                fprintf(stderr, "clock_nanosleep: %s\n",
                        strerror(ret));
            break;
        }

        if (!running)
            break;

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        add_jitter(timespec_to_ns(actual) -
                   timespec_to_ns(deadline));

        sample.timestamp_ms =
            (uint64_t)actual.tv_sec * 1000U +
            (uint64_t)actual.tv_nsec / 1000000U;

        random_state ^= random_state << 13;
        random_state ^= random_state >> 17;
        random_state ^= random_state << 5;

        sample.temperature =
            15.0f + (float)(random_state % 31U);

        random_state ^= random_state << 13;
        random_state ^= random_state >> 17;
        random_state ^= random_state << 5;

        sample.humidity =
            30.0f + (float)(random_state % 61U);

        if (buffer_push(&sample) != 0) {
            fprintf(stderr,
                    "WARNING: Sensor buffer full\n");
        }

        advance_deadline(&deadline, interval);
    }

#else

    int fd = open(DEVICE_PATH, O_RDONLY);

    if (fd < 0) {
        perror("Sensor: cannot open device");
        running = 0;
        pthread_cond_broadcast(&buffer_not_empty);
        print_jitter_report();
        return NULL;
    }

    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        perror("Sensor: ioctl failed");
        close(fd);
        running = 0;
        pthread_cond_broadcast(&buffer_not_empty);
        print_jitter_report();
        return NULL;
    }

    char line[128];

    while (running) {
        int ret = wait_until(&deadline);

        if (ret != 0) {
            if (running)
                fprintf(stderr, "clock_nanosleep: %s\n",
                        strerror(ret));
            break;
        }

        if (!running)
            break;

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        add_jitter(timespec_to_ns(actual) -
                   timespec_to_ns(deadline));

        ssize_t bytes_read = read(fd, line, sizeof(line) - 1);

        if (bytes_read < 0) {
            if (errno == EINTR && !running)
                break;

            perror("Sensor: read failed");
            break;
        }

        if (bytes_read == 0) {
            advance_deadline(&deadline, interval);
            continue;
        }

        line[bytes_read] = '\0';

        SensorData sample;
        unsigned long long timestamp;

        if (sscanf(line, "%llu,%f,%f",
                   &timestamp,
                   &sample.temperature,
                   &sample.humidity) != 3) {
            fprintf(stderr, "Invalid CSV: %s\n", line);
            advance_deadline(&deadline, interval);
            continue;
        }

        sample.timestamp_ms = (uint64_t)timestamp;

        if (buffer_push(&sample) != 0) {
            fprintf(stderr,
                    "WARNING: Sensor buffer full\n");
        }

        advance_deadline(&deadline, interval);
    }

    close(fd);

#endif

    print_jitter_report();

    printf("Sensor: thread stopped\n");
    return NULL;
}
