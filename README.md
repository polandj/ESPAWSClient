# ESPAWSClient
Simple AWS client v4 library for the ESP8266.  This implements the workflow at https://docs.aws.amazon.com/apigateway/api-reference/signing-requests/.

## Dependencies

 * [TimeLib](https://github.com/PaulStoffregen/Time)
 * [CryptoSuite](https://github.com/jjssoftware/Cryptosuite)

## Usage

Suppose you wanted to do a signed POST to an AWS gateway at https://ivsxai2qus.execute-api.us-east-1.amazonaws.com/

```C++
const char *host = "ivsxai2qus";
const char *service = "execute-api";
const char *key = "YOUR_AWS_IAM_KEY";
const char *host = "YOUR_AWS_IAM_SECRET";
const char *uri = "/YOUR_POST_URI";
const char *payload = "{\"key\":true}";

ESPAWSClient aws = ESPAWSClient(service, key, secret, host);
AWSResponse resp = aws.doPost(uri, payload);
if (resp.status != 200) {
    Serial.printf("Error: %s", resp.body.c_str());
    Serial.println();
}
```

## Custom Domain
If you have configured a custom domain for your AWS endpoint, you can specify it with the **setCustomFQDN** method.

```C++
ESPAWSClient aws = ESPAWSClient(service, key, secret, host);
aws.setCustomFQDN("api.domain.com");
```

## Verify SSL fingerprint
You can force SSL fingerprint verification by using the **setFingerPrint** method.

```C++
ESPAWSClient aws = ESPAWSClient(service, key, secret, host);
aws.setFingerPrint("CC AA 48...");
```

## Verify Certs / Client certificates
The ESPAWSClient class inherits from the [WiFiClientSecure](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h) class, so any methods there can be called as well.

```C++
ESPAWSClient aws = ESPAWSClient(service, key, secret, host);
aws.setCACert(...);
```

# Caveats

## Time
You must have the proper **system time** set on your device.  The library uses the *gettimeofday* internally to get the time.  The AWS signed requests are only valid for a certain amount of time, so if AWS will reject your signatures if your time is wrong.  Note that many Arduino NTP libraries do not call *settimeofday* to set the system time.  See the example sketch for a full example with NTP.

## Memory
The ESP8266 typically only has 80K of RAM total.  AWS uses somewhat large certs and TLS 1.2.  Just doing the connection requires around 20K of available HEAP.  More if you add in certificate validation.  If you're adding this to an existing Arduino program, check you're not running out of RAM.

# Examples
See the [examples directory](https://github.com/polandj/ESPAWSClient/tree/master/examples) in the source to get started.
