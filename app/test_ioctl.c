/*
 * Assignment 02 - Test SMS Sensor ioctl
 * Author: Do Phuoc Hung - SE204688
 * Purpose: Test SMS_SET_INTERVAL ioctl.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE_PATH "/dev/sms_sensor"
#define SMS_SET_INTERVAL _IOW('S', 1, unsigned int)

int main(void)
{
    int fd;
    unsigned int interval;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Cannot open /dev/sms_sensor");
        return 1;
    }

    interval = 200;
    if (ioctl(fd, SMS_SET_INTERVAL, &interval) == 0) {
        printf("PASS: Set interval to 200 ms\n");
    } else {
        perror("FAIL: Set interval to 200 ms");
        close(fd);
        return 1;
    }

    interval = 50;
    if (ioctl(fd, SMS_SET_INTERVAL, &interval) < 0) {
        printf("PASS: Interval 50 ms rejected (errno=%d)\n", errno);
    } else {
        printf("FAIL: Interval 50 ms should be rejected\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
