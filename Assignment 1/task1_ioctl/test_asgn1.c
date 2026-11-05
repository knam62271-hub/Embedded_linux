#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    int new_mode;
    int len;
    char version[64];
    char data[1024];
    struct asgn1_stats stats;

    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("=== ASGN1 DRIVER TEST ===\n");

    /* Test Echo mode */
    new_mode = 1;
    if (ioctl(fd, ASGN1_SET_MODE, &new_mode) < 0)
        perror("SET_MODE Echo");

    write(fd, "Hello Assignment 01", 19);

    len = read(fd, data, sizeof(data) - 1);
    if (len >= 0) {
        data[len] = '\0';
        printf("Echo mode data: %s\n", data);
    }
close(fd);

fd = open("/dev/asgn1", O_RDWR);
if (fd < 0) {
    perror("open");
    return 1;
}
    /* Test Kernel mode */
    new_mode = 0;
    if (ioctl(fd, ASGN1_SET_MODE, &new_mode) < 0)
        perror("SET_MODE Kernel");

    write(fd, "Hello Kernel Mode", 17);

    
    len = read(fd, data, sizeof(data) - 1);
    if (len >= 0) {
        data[len] = '\0';
        printf("Kernel mode data: %s\n", data);
    }

    /* Test GET_STATS */
    if (ioctl(fd, ASGN1_GET_STATS, &stats) == 0) {
        printf("\n--- Driver Statistics ---\n");
        printf("Open count      : %d\n", stats.open_count);
        printf("Write count     : %d\n", stats.write_count);
        printf("Read count      : %d\n", stats.read_count);
        printf("Buffer length   : %d\n", stats.buffer_len);
        printf("Last write size : %d\n", stats.last_write_size);
    } else {
        perror("GET_STATS");
    }

    /* Test GET_VERSION */
    memset(version, 0, sizeof(version));
    if (ioctl(fd, ASGN1_GET_VERSION, version) == 0)
        printf("\nDriver version: %s\n", version);
    else
        perror("GET_VERSION");

    /* Test RESET_BUFFER */
    if (ioctl(fd, ASGN1_RESET_BUFFER) == 0) {
        printf("\nBuffer reset: OK\n");
        len = read(fd, data, sizeof(data) - 1);
        printf("Read after reset: %d bytes\n", len);
    } else {
        perror("RESET_BUFFER");
    }

    close(fd);

    return 0;
}
