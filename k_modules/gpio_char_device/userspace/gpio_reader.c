/* 
 * User space companion to gpio_char_dev.ko
 *
 * Blocks on read() from /dev/gpio_event and prints each button
 * press event as it arrives. Runs until interrupted with Ctrl+C.
 */
// ==============================================================================
// including the required header files

/** standard C header files */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/** POSIX header files */
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
// ==============================================================================
// variable, macro and constant definitions

#define DEVICE_PATH "/dev/gpio_event"

static volatile int running = 1;

// ==============================================================================
// user interface main functions:

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    int fd;
    uint8_t count;
    ssize_t ret;

    signal(SIGINT, handle_sigint);

    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open " DEVICE_PATH);
        return EXIT_FAILURE;
    }

    printf("Listening on %s — press the button (Ctrl+C to quit)\n\n",
           DEVICE_PATH);

    while (running) {
        ret = read(fd, &count, 1);
        if (ret < 0) {
            /* read() was interrupted by our signal handler — clean exit */
            break;
        }
        if (ret == 0) {
            fprintf(stderr, "Unexpected EOF from %s\n", DEVICE_PATH);
            break;
        }

        printf("Button press detected — cumulative count: %u\n", count);
        fflush(stdout);
    }

    close(fd);
    printf("\nExiting.\n");
    return EXIT_SUCCESS;
}