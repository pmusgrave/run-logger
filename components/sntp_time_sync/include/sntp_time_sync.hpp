#ifndef SNTP_TIME_SYNC
#define SNTP_TIME_SYNC

#include <ctime>

void sntp_synchronize_time(struct timeval *tv);
void time_sync_notification_cb(struct timeval *tv);
void obtain_time(void);
void initialize_sntp(void);

#endif