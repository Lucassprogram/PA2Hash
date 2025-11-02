#include "timestamp.h"

#include <sys/time.h>

long long current_timestamp_microseconds(void) {
    struct timeval te;
    gettimeofday(&te, NULL);
    return (long long)te.tv_sec * 1000000LL + te.tv_usec;
}
