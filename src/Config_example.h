// Rename this file to Config.h and fill in the missing values

#include <Arduino.h>

#define SECOND 1000
#define MINUTE 60 * SECOND
#define HOUR 60 * MINUTE

static const int servoPin = 13;
static const uint64_t fetch_interval = 2 * HOUR;
static const uint64_t evaluate_interval = 1 * MINUTE;

static const char* WIFI_SSID = "Your WiFi SSID";
static const char* WIFI_PASSWORD = "Your WiFi Password";

static const char *ntpServer1 = "de.pool.ntp.org";
static const char *timeZone = "CET-1CEST,M3.5.0/03,M10.5.0/03"; // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

static const char *tibberApi = "api.tibber.com";
static const uint16_t tibberPort = 443;
// https://www.grc.com/fingerprints.htm
static const char *tibberSSLfingerprint = "0B 06 41 39 B8 51 EB E0 D3 A0 6B 6A 5B C7 5A 70 28 AD A8 5F";
static const char *tibberToken = "Your Tibber Token";
static const char *tibberQuerry = "{\"query\": \"{ viewer { homes { currentSubscription{ priceInfo{ current{ total startsAt level } today { total startsAt level } tomorrow { total startsAt level } } } } } } \" }";



