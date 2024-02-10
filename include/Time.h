#pragma once

#include <Arduino.h>

void time_setup();
const char *Time_getStateString();
bool time_loop();
void printTime();
int secs_to_tm(long long t, struct tm *tm);
void getTimeAdv(struct tm *tm, unsigned long offset);
void getTime(struct tm *tm);
void getStartupTime(struct tm *tm);
void sendNTPpacket(IPAddress &address);
