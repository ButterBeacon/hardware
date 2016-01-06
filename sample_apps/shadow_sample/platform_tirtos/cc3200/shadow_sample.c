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
#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_json_data.h"
#include "aws_iot_config.h"
#include "aws_iot_mqtt_interface.h"

/*
 * The goal of this sample application is to demonstrate the capabilities of
 * shadow.
 * This device(say Connected Window) will open the window of a room based on
 * temperature.
 * It can report to the Shadow the following parameters:
 *  1. temperature of the room (double)
 *  2. status of the window (open or close)
 * It can act on commands from the cloud. In this case it will open or close
 * the window based on the json object "windowOpen" data[open/close]
 *
 * The two variables from a device's perspective are double temperature and
 * bool windowOpen
 * The device needs to act on only on windowOpen variable, so we will create
 * a primitiveJson_t object with callback
 *
 * The Json Document in the cloud will be
 * {
 *   "reported": {
 *     "temperature": 0,
 *     "windowOpen": false
 *   },
 *   "desired": {
 *     "windowOpen": false
 *   }
 * }
 */

#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT
#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200

char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
uint8_t numPubs = 5;

static void simulateRoomTemperature(float *pRoomTemperature)
{
    static float deltaChange;

    if (*pRoomTemperature >= ROOMTEMPERATURE_UPPERLIMIT) {
        deltaChange = -0.5f;
    } else if(*pRoomTemperature <= ROOMTEMPERATURE_LOWERLIMIT) {
        deltaChange = 0.5f;
    }

    *pRoomTemperature+= deltaChange;
}


void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action,
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

void windowActuate_Callback(const char *pJsonString, uint32_t JsonStringDataLen,
      jsonStruct_t *pContext)
{
    if (pContext != NULL) {
        INFO("Delta - Window state changed to %d", *(bool *)(pContext->pData));
    }
}

void runAWSClient(void)
{
    IoT_Error_t rc = NONE_ERROR;

    MQTTClient_t mqttClient;
    aws_iot_mqtt_init(&mqttClient);

    char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer)
            / sizeof(JsonDocumentBuffer[0]);
    float temperature = 0.0;

    bool windowOpen = false;
    jsonStruct_t windowActuator;
    windowActuator.cb = windowActuate_Callback;
    windowActuator.pData = &windowOpen;
    windowActuator.pKey = "windowOpen";
    windowActuator.type = SHADOW_JSON_BOOL;

    jsonStruct_t temperatureHandler;
    temperatureHandler.cb = NULL;
    temperatureHandler.pKey = "temperature";
    temperatureHandler.pData = &temperature;
    temperatureHandler.type = SHADOW_JSON_FLOAT;

    INFO("\nAWS IoT SDK Version(dev) %d.%d.%d-%s\n", VERSION_MAJOR,
            VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    DEBUG("Using rootCA %s", AWS_IOT_ROOT_CA_FILENAME);
    DEBUG("Using clientCRT %s", AWS_IOT_CERTIFICATE_FILENAME);
    DEBUG("Using clientKey %s", AWS_IOT_PRIVATE_KEY_FILENAME);

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

    INFO("Shadow Connect");
    rc = aws_iot_shadow_connect(&mqttClient, &sp);

    if (NONE_ERROR != rc) {
        ERROR("Shadow Connection Error %d", rc);
    }

    rc = aws_iot_shadow_register_delta(&mqttClient, &windowActuator);

    if (NONE_ERROR != rc) {
        ERROR("Shadow Register Delta Error");
    }
    temperature = STARTING_ROOMTEMPERATURE;

    /* loop and publish a change in temperature */
    while (NONE_ERROR == rc) {
        rc = aws_iot_shadow_yield(&mqttClient, 200);
        INFO("\n==========================================================\n");
        INFO("On Device: window state %s", windowOpen?"true":"false");
        simulateRoomTemperature(&temperature);

        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer,
                sizeOfJsonDocumentBuffer);
        if (rc == NONE_ERROR) {
            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer,
                    sizeOfJsonDocumentBuffer, 2, &temperatureHandler,
                    &windowActuator);
            if (rc == NONE_ERROR) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer,
                        sizeOfJsonDocumentBuffer);
                if (rc == NONE_ERROR) {
                    INFO("Update Shadow: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&mqttClient,
                            AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
                            ShadowUpdateStatusCallback, NULL, 4, true);
                }
            }
        }
        INFO("\n==========================================================\n");
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
