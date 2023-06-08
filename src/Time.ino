#include <Arduino.h>

#include "Time.h"

enum statusType
{
    Idle,
    Sent,
    Received,
    Pause
};

IPAddress timeServerIP; // time.nist.gov NTP server address
const char *ntpServerName = "time.nist.gov";
unsigned int localPort = 2390;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets
WiFiUDP Udp;                        // A UDP instance to let us send and receive packets over UDP

uint32_t retries = 0;
unsigned long lastSent = 0;
unsigned long timeReference = 0;
bool time_valid = false;
unsigned long secsSince1900 = 0;
unsigned long setup_time_offset = 2;

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400 * (31 + 29))
#define DAYS_PER_400Y (365 * 400 + 97)
#define DAYS_PER_100Y (365 * 100 + 24)
#define DAYS_PER_4Y (365 * 4 + 1)

statusType currentStatus = Idle;

void time_setup()
{
    Udp.begin(localPort);
    currentStatus = Idle;
    lastSent = 0;
    timeReference = 0;
    memset(packetBuffer, 0x00, sizeof(packetBuffer));
}

const char *Time_getStateString()
{
    static char retString[64];
    const char *state = "";

    switch (currentStatus)
    {
    default:
        state = "Unknown state";
        break;

    case Idle:
        state = "Idle";
        break;

    case Sent:
        state = "Sent";
        break;

    case Received:
        state = "Received";
        break;

    case Pause:
        state = "Pause";
        break;
    }

    snprintf(retString, sizeof(retString), "%s, ref: %lu, last: %lu, retries: %u, millis(): %lu", state, timeReference, lastSent, retries, millis());

    return retString;
}

bool time_loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    switch (currentStatus)
    {
    default:
        Serial.println("[NTP] Unknown state");
        currentStatus = Idle;
        break;

    case Idle:
        if (!time_valid || millis() - lastSent > 1000 * 60 * 60)
        {
            Serial.println("[NTP] Sending request");

            lastSent = millis();
            currentStatus = Sent;
            WiFi.hostByName(ntpServerName, timeServerIP);
            sendNTPpacket(timeServerIP); // send an NTP packet to a time server
        }
        break;

    case Sent:
        if (millis() - lastSent > 1000 * 10)
        {
            Serial.println("[NTP] No reply, resend");
            if (retries < 10)
            {
                retries++;
                currentStatus = Idle;
            }
            else
            {
                currentStatus = Pause;
            }
        }
        else if (Udp.parsePacket())
        {
            timeReference = millis();
            currentStatus = Received;
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        }
        break;

    case Received:
    {
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        secsSince1900 = highWord << 16 | lowWord;

        printTime();

        if (!time_valid)
        {
            time_valid = true;
        }

        retries = 0;
        currentStatus = Idle;
        break;
    }

    case Pause:
        if (millis() - lastSent > 1000 * 60 * 2)
        {
            currentStatus = Idle;
        }
        break;
    }

    return false;
}

void printTime()
{
    struct tm tm;
    getTime(&tm);

    Serial.printf("[NTP] The time is: %02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int secs_to_tm(long long t, struct tm *tm)
{
    long long days, secs;
    int remdays, remsecs, remyears;
    int qc_cycles, c_cycles, q_cycles;
    int years, months;
    int wday, yday, leap;
    static const char days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};

    secs = t - LEAPOCH;
    days = secs / 86400;
    remsecs = secs % 86400;
    if (remsecs < 0)
    {
        remsecs += 86400;
        days--;
    }

    wday = (3 + days) % 7;
    if (wday < 0)
        wday += 7;

    qc_cycles = days / DAYS_PER_400Y;
    remdays = days % DAYS_PER_400Y;
    if (remdays < 0)
    {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / DAYS_PER_100Y;
    if (c_cycles == 4)
        c_cycles--;
    remdays -= c_cycles * DAYS_PER_100Y;

    q_cycles = remdays / DAYS_PER_4Y;
    if (q_cycles == 25)
        q_cycles--;
    remdays -= q_cycles * DAYS_PER_4Y;

    remyears = remdays / 365;
    if (remyears == 4)
        remyears--;
    remdays -= remyears * 365;

    leap = !remyears && (q_cycles || !c_cycles);
    yday = remdays + 31 + 28 + leap;
    if (yday >= 365 + leap)
        yday -= 365 + leap;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400 * qc_cycles;

    for (months = 0; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    tm->tm_year = years + 100;
    tm->tm_mon = months + 2;
    if (tm->tm_mon >= 12)
    {
        tm->tm_mon -= 12;
        tm->tm_year++;
    }
    tm->tm_mday = remdays;
    tm->tm_wday = wday;
    tm->tm_yday = yday;

    tm->tm_hour = remsecs / 3600;
    tm->tm_min = remsecs / 60 % 60;
    tm->tm_sec = remsecs % 60;

    return 0;
}

void getTimeAdv(struct tm *tm, unsigned long offset)
{
    unsigned long epoch = secsSince1900 - 2208988800UL;

    long secs = ((long)offset - (long)timeReference) / 1000;
    epoch += 60 * 60 * setup_time_offset;
    epoch += secs;

    secs_to_tm(epoch, tm);
}

void getTime(struct tm *tm)
{
    getTimeAdv(tm, millis());
}

void getStartupTime(struct tm *tm)
{
    getTimeAdv(tm, 0);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
    memset(packetBuffer, 0, NTP_PACKET_SIZE);

    // Initialize values needed to form NTP request
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}
