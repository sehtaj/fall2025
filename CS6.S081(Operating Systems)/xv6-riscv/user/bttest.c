// bttest.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    printf("Running bttest...\n");

    // Call sleep so that sys_sleep is invoked
    sleep(1);

    printf("bttest done\n");
    exit(0);
}