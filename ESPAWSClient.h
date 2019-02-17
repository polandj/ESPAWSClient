/*
 * ESPAWSClient
 * Copyright 2018 - Jonathan Poland
 * 
 * A simple client for AWS v4 request signing via the ESP8266.
 *
 * It implements the workflow described at:
 *  https://docs.aws.amazon.com/apigateway/api-reference/signing-requests/
 *
 * It depends on:
 *  https://github.com/PaulStoffregen/Time
 *  https://github.com/jjssoftware/Cryptosuite
 *
 * The ESP device must have a valid time set (e.g. NTP) for this to work.
 *
 */
#ifndef ESPAWSCLIENT_H_
#define ESPAWSCLIENT_H_

#include <ESP8266WiFi.h>

typedef enum : uint8_t {
    CAPTURE_HEADERS = 1,
    CAPTURE_BODY = 2,
    CAPTURE_BODY_ON_ERROR = 4
} AWSResponseFieldMask;

class AWSResponse {
    public:
        AWSResponse() {
            status = 0;
            contentLength = 0;
        }
        uint16_t status;
        String contentType;
        int contentLength;
        String headers;
        String body;
};

class ESPAWSClient : public WiFiClientSecure {
    public:
        ESPAWSClient(String service, String key, String secret, String host, 
                String region="us-east-1", String TLD="amazonaws.com"); 
        void setCustomFQDN(String fqdn);
        void setResponseFields(AWSResponseFieldMask fields);
        String createRequest(String method, String uri, String payload="", 
                String contentType="application/json", String queryString="");
        AWSResponse doGet(String uri, String queryString="");
        AWSResponse doPost(String uri, String payload, String contentType="application/json", String queryString="");
        AWSResponse send(const String request);
    protected:
        String _awsHost;
        String _awsRegion;
        String _awsTLD;
        String _awsService;
        String _awsKey;
        String _awsSecret;
        String _customFQDN;
        String _signedHeaders = "content-type;host;x-amz-content-sha256;x-amz-date";
        AWSResponseFieldMask _responseFields = CAPTURE_BODY_ON_ERROR;

        String FQDN();
        String hexHash(uint8_t *hash);
        String createCanonicalHeaders(String contentType, String date, String time, String payloadHash);
        String createRequestHeaders(String contentType, String date, String time, String payload, String payloadHash, String signature);
        String createStringToSign(String canonical_request, String date, String time);
        String createCanonicalRequest(String method, String uri, String date, String time, String payloadHash, String queryString="", String contentType="application/json");
        String createSignature(String toSign, String date);
};

#endif
