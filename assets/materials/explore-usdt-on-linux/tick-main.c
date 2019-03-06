#include <stdio.h>
#include <unistd.h>
#include "tick-dtrace.h"

int
main(int argc, char *argv[])
{
        int i;

        while(1) {
                i++;
                DTRACE_PROBE1(tick, loop1, i);
                if (TICK_LOOP2_ENABLED()) {
                        DTRACE_PROBE1(tick, loop2, i);
                }
                printf("tick: %d\n", i);
                sleep(5);
        }

        return (0);
}
