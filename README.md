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

# Examples
See the [examples directory](https://github.com/polandj/ESPAWSClient/tree/master/examples) in the source to get started.
