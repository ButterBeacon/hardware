/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <string.h>

#include <ti/sysbios/knl/Task.h>

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_interface.h"
#include "aws_iot_shadow_interface.h"
#include "aws_iot_config.h"

/*
 * The purpose of the application is to echo the contents of the message
 * published on the delta topic
 * For More details follow the console wizard
 * Anything received on the delta topic will be sent back in reported
 *
 * This example will not use any of the json builder/helper functions provided
 * in the aws_iot_shadow_json_data.h.
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
bool messageArrivedOnDelta = false;

/*
 * @note The delta message is always sent on the "state" key in the json
 * @note Any time messages are bigger than AWS_IOT_MQTT_RX_BUF_LEN the
 * underlying MQTT library will ignore it. The maximum size of the message
 * that can be received is limited to the AWS_IOT_MQTT_RX_BUF_LEN
 */
char stringToEchoDelta[SHADOW_MAX_SIZE_OF_RX_BUFFER];

/* Shadow Callback for receiving the delta */
void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength,
        jsonStruct_t *pJsonStruct_t);

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action,
        Shadow_Ack_Status_t status, const char *pReceivedJsonDocument,
        void *pContextData);

void runAWSClient(void)
{
    IoT_Error_t rc = NONE_ERROR;

    MQTTClient_t mqttClient;
    aws_iot_mqtt_init(&mqttClient);

    ShadowParameters_t sp = ShadowParametersDefault;
    sp.pMyThingName = AWS_IOT_MY_THING_NAME;
    sp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
    sp.pHost = HostAddress;
    sp.port = port;
    sp.pClientCRT = AWS_IOT_CERTIFICATE_FILENAME;
    sp.pClientKey = AWS_IOT_PRIVATE_KEY_FILENAME;
    sp.pRootCA = AWS_IOT_ROOT_CA_FILENAME;

    INFO("Shadow Init");
    rc = aws_iot_shadow_init(&mqttClient);
    if (NONE_ERROR != rc) {
        ERROR("Shadow Connection Error");
        return;
    }

    INFO("Shadow Connect");
    rc = aws_iot_shadow_connect(&mqttClient, &sp);
    if (NONE_ERROR != rc) {
        ERROR("Shadow Connection Error");
        return;
    }

    jsonStruct_t deltaObject;
    deltaObject.pData = stringToEchoDelta;
    deltaObject.pKey = "state";
    deltaObject.type = SHADOW_JSON_OBJECT;
    deltaObject.cb = DeltaCallback;

    /*
     * Register the jsonStruct object
     */
    rc = aws_iot_shadow_register_delta(&mqttClient, &deltaObject);

    /* Now wait in the loop to receive any message sent from the console */
    while (rc == NONE_ERROR) {
        /*
         * Lets check for the incoming messages for 200 ms.
         */
        rc = aws_iot_shadow_yield(&mqttClient, 200);

        if (messageArrivedOnDelta) {
            INFO("\nSending delta message back %s\n", stringToEchoDelta);
            rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME,
                    stringToEchoDelta, UpdateStatusCallback, NULL, 2, true);
            messageArrivedOnDelta = false;
        }

        /* sleep for some time in seconds */
        Task_sleep(1000);
    }

    if (NONE_ERROR != rc) {
        ERROR("An error occurred in the loop %d", rc);
    }

    INFO("Disconnecting");
    rc = aws_iot_shadow_disconnect(&mqttClient);

    if (NONE_ERROR != rc) {
        ERROR("Disconnect error %d", rc);
    }
}

bool buildJSONForReported(char *pJsonDocument, size_t maxSizeOfJsonDocument,
        const char *pReceivedDeltaData, uint32_t lengthDelta)
{
    int32_t ret;

    if (pJsonDocument == NULL) {
        return false;
    }

    char tempClientTokenBuffer[MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE];

    if (aws_iot_fill_with_client_token(tempClientTokenBuffer,
            MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE) != NONE_ERROR) {
        return false;
    }

    ret = snprintf(pJsonDocument, maxSizeOfJsonDocument,
            "{\"state\":{\"reported\":%.*s}, \"clientToken\":\"%s\"}",
            lengthDelta, pReceivedDeltaData, tempClientTokenBuffer);

    if (ret >= maxSizeOfJsonDocument || ret < 0) {
        return false;
    }

    return true;
}

void DeltaCallback(const char *pJsonValueBuffer, uint32_t valueLength,
        jsonStruct_t *pJsonStruct_t)
{
    DEBUG("Received Delta message %.*s", valueLength, pJsonValueBuffer);

    if (buildJSONForReported(stringToEchoDelta, SHADOW_MAX_SIZE_OF_RX_BUFFER,
            pJsonValueBuffer, valueLength)) {
        messageArrivedOnDelta = true;
    }
}

void UpdateStatusCallback(const char *pThingName, ShadowActions_t action,
        Shadow_Ack_Status_t status, const char *pReceivedJsonDocument,
        void *pContextData)
{
    if (status == SHADOW_ACK_TIMEOUT) {
        INFO("Update Timeout--");
    } else if (status == SHADOW_ACK_REJECTED) {
        INFO("Update RejectedXX");
    } else if (status == SHADOW_ACK_ACCEPTED) {
        INFO("Update Accepted !!");
    }
}
