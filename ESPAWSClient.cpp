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
#include "ESPAWSClient.h"
#include <sha256.h>
#include <TimeLib.h>
#include <sys/time.h>

ESPAWSClient::ESPAWSClient(String service, String key, String secret, String host,
        String region, String TLD) {
    _awsHost = host;
    _awsRegion = region;
    _awsTLD = TLD;
    _awsService = service;
    _awsKey = key;
    _awsSecret = secret;
}

void ESPAWSClient::setCustomFQDN(String fqdn) {
    _customFQDN = fqdn;
}

void ESPAWSClient::setFingerPrint(String fp) {
    _fingerPrint = fp;
}

void ESPAWSClient::setResponseFields(AWSResponseFieldMask fields) {
    _responseFields = fields;
}

String ESPAWSClient::createRequest(String method, String uri, String payload, 
        String contentType, String queryString) {
    char dateBuf[9], timeBuf[7];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(dateBuf, sizeof(dateBuf), "%4d%02d%02d", year(tv.tv_sec), month(tv.tv_sec), day(tv.tv_sec));
    snprintf(timeBuf, sizeof(timeBuf), "%02d%02d%02d", hour(tv.tv_sec), minute(tv.tv_sec), second(tv.tv_sec));
    Sha256.init();
    Sha256.print(payload);
    String payloadHash = hexHash(Sha256.result());
    String date(dateBuf);
    String time(timeBuf);

    String canonical_request = createCanonicalRequest(method, uri, date, time, payloadHash, queryString, contentType);
    String string_to_sign = createStringToSign(canonical_request, date, time);
    String signature = createSignature(string_to_sign, date);
    String headers = createRequestHeaders(contentType, date, time, payload, payloadHash, signature);

    String retval;
    retval += method + " " + "https://" + FQDN() + uri + " " + "HTTP/1.1\r\n";
    retval += headers + "\r\n";
    retval += payload + "\r\n\r\n";
    return retval;
}

AWSResponse ESPAWSClient::doGet(String uri, String queryString) {
    String request = createRequest("GET", uri, queryString=queryString);
    return send(request);
}

AWSResponse ESPAWSClient::doPost(String uri, String payload, String contentType, String queryString) {
    String request = createRequest("POST", uri, payload, contentType, queryString);
    return send(request);
}

AWSResponse ESPAWSClient::send(const String request) {
    AWSResponse response;
    if (connect(FQDN(), 443)) {
        if (_fingerPrint.length() && !verify(_fingerPrint.c_str(), FQDN().c_str())) {
            response.status = 500;
            response.contentType = F("text/plain");
            response.body = F("Fingerprint mismatch");
        } else {
            // Send our request
            print(request);
            // Read headers
            while (connected()) {
                String line = readStringUntil('\n');
                if (line.startsWith(F("HTTP/1.1 "))) {
                    response.status = line.substring(9, 12).toInt();
                } else if (line.startsWith(F("Content-Type:"))) {
                    response.contentType = line.substring(14);
                } else if (line.startsWith(F("Content-Length:"))) {
                    response.contentLength = line.substring(15).toInt();
                } else if (line == "\r") {
                    break;
                } else if (_responseFields & CAPTURE_HEADERS) {
                    response.headers.concat(line);
                }
            }
            // Read body
            bool saveBody = (_responseFields & CAPTURE_BODY || response.status >= 400 & CAPTURE_BODY_ON_ERROR);
            if (saveBody) {
                response.body.reserve(response.contentLength);
            }
            while (connected()) {
                while (available()) {
                    String tmp = readString();
                    if (saveBody) {
                        response.body.concat(tmp);
                    }
                }
            }
        }
    } else {
        response.status = 500;
        response.contentType = F("text/plain");
        response.body = F("Connection failure");
    }
    stop();
    return response;
}

String ESPAWSClient::FQDN() {
    String retval;
    if (_customFQDN.length() > 0) {
        retval = _customFQDN;
    } else {
        retval = _awsHost + "." + _awsService + "." + _awsRegion + "." + _awsTLD;
    }
    return retval;
}

String ESPAWSClient::hexHash(uint8_t *hash) {
    char hashStr[(HASH_LENGTH * 2) + 1];
    for (int i = 0; i < HASH_LENGTH; ++i) {
        sprintf(hashStr+ 2 * i, "%02lx", 0xff & (unsigned long) hash[i]);
    }
    return String(hashStr);
}

String ESPAWSClient::createCanonicalHeaders(String contentType, String date, String time, String payloadHash) {
    String retval;
    retval += "content-type:" + contentType + "\n";
    retval += "host:" + FQDN() + "\n";
    retval += "x-amz-content-sha256:" + payloadHash + "\n";
    retval += "x-amz-date:" + date + "T" + time + "Z\n\n";
    return retval;
}

String ESPAWSClient::createRequestHeaders(String contentType, String date, String time, String payload, String payloadHash, String signature) {
    String retval;
    retval += "Content-Type: " + contentType + "\r\n";
    retval += "Connection: close\r\n";
    retval += "Content-Length: " + String(payload.length()) + "\r\n";
    retval += "Host: " + FQDN() + "\r\n";
    retval += "x-amz-content-sha256: " + payloadHash + "\r\n";
    retval += "x-amz-date: " + date + "T" + time + "Z\r\n";
    retval += "Authorization: AWS4-HMAC-SHA256 Credential=" + _awsKey + "/" + \
               String(date) + "/" + _awsRegion + "/" + _awsService + "/aws4_request,SignedHeaders=" + \
               _signedHeaders + ",Signature=" + signature + "\r\n";
    return retval;
}

String ESPAWSClient::createStringToSign(String canonical_request, String date, String time) {
    Sha256.init();
    Sha256.print(canonical_request);
    String hash = hexHash(Sha256.result());

    String retval;
    retval += "AWS4-HMAC-SHA256\n";
    retval += date + "T" + time + "Z\n";
    retval += date + "/" + _awsRegion + "/" + _awsService + "/aws4_request\n";
    retval += hash;
    return retval;
}

String ESPAWSClient::createCanonicalRequest(String method, String uri, String date, String time, String payloadHash, String queryString, String contentType) {
    String retval;
    retval += method + "\n";
    retval += uri + "\n";
    retval += queryString + "\n";
    String headers = createCanonicalHeaders(contentType, date, time, payloadHash);
    retval += headers + _signedHeaders + "\n";
    retval += payloadHash;
    return retval;
}

String ESPAWSClient::createSignature(String toSign, String date) {
    String key = "AWS4" + _awsSecret;

    Sha256.initHmac((uint8_t*)key.c_str(), key.length()); 
    Sha256.print(date);
    uint8_t* hash = Sha256.resultHmac();

    Sha256.initHmac(hash, HASH_LENGTH);
    Sha256.print(_awsRegion);
    hash = Sha256.resultHmac();

    Sha256.initHmac(hash, HASH_LENGTH);
    Sha256.print(_awsService);
    hash = Sha256.resultHmac();

    Sha256.initHmac(hash, HASH_LENGTH);
    Sha256.print("aws4_request");
    hash = Sha256.resultHmac();

    Sha256.initHmac(hash, HASH_LENGTH);
    Sha256.print(toSign);
    hash = Sha256.resultHmac();

    return hexHash(hash);
}
