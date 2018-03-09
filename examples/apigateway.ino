#include "ESPAWSClient.h"
#include <ESP8266WiFi.h>
#include <coredecls.h>

#define TZ              -5      // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

// Local WiFi configuration
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// AWS API gateway configuration
// Suppose your API gateway URI is:
//   https://ivsxai2qus.execute-api.us-east-1.amazonaws.com/
// Then:
//   host = "ivsxai2qus"
//   service = "execute-api"
//   region = "us-east-1" (the default)
//   TLD = "amazonaws.com" (the default)
const char *service = "execute-api";
const char *host = "YOUR_API_HOST";

// AWS IAM configuration
const char *key = "YOUR_IAM_KEY";
const char *secret = "YOUR_IAM_SECRET";

// Optional custom domain (if used, "host" field set is not used)
const char *customDomain = "YOUR_CUSTOM_DOMAIN"; // e.g. api.domain.com

bool cbtime_set = false;

void time_is_set (void)
{
  cbtime_set = true;
}

void setup() {
  Serial.begin(115200);
  delay(10);
  settimeofday_cb(time_is_set);

  // Connect to WAP
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(".");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  // Get time
  Serial.print("Getting time.");
  configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  while(!cbtime_set) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("done");
}

void loop(){
  static uint32_t requests = 0;
  ESPAWSClient aws = ESPAWSClient(service, key, secret, host);
  aws.setCustomFQDN(customDomain);
  char *uuid = "UUID-SAMPLE";
  char uri[50], json[70];
  sprintf(uri, "/device/%s", uuid);
  sprintf(json, "{\"ip\": \"%s\", \"id\":\"%X\"}", WiFi.localIP().toString().c_str(), ESP.getChipId()); 
  AWSResponse resp = aws.doPost(uri, json);
  if (resp.status != 200) {
    Serial.printf("Error: %s", resp.body.c_str());
    Serial.println();
  }
  Serial.printf("Uptime: %u", millis()/1000); Serial.println();
  Serial.printf("Requests: %u", requests++); Serial.println();
  Serial.printf("Free memory: %d", ESP.getFreeHeap()); Serial.println();
  Serial.println();

  delay(30000);
}
