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
#include "aws_iot_config.h"

char HostAddress[255] = AWS_IOT_MQTT_HOST;
uint32_t port = AWS_IOT_MQTT_PORT;
uint32_t publishCount = 0;

int MQTTcallbackHandler(MQTTCallbackParams params)
{
   INFO("Subscribe callback");
   INFO("%.*s\t%.*s",(int)params.TopicNameLen, params.pTopicName,
            (int)params.MessageParams.PayloadLen,
            (char*)params.MessageParams.pPayload);

   return (0);
}

void disconnectCallbackHandler(void)
{
    WARN("MQTT Disconnect");
}

void runAWSClient(void)
{
    IoT_Error_t rc = NONE_ERROR;
    int32_t i = 0;
    bool infinitePublishFlag = true;

    INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR,
            VERSION_PATCH, VERSION_TAG);
    MQTTConnectParams connectParams = MQTTConnectParamsDefault;
    connectParams.KeepAliveInterval_sec = 10;
    connectParams.isCleansession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = "CSDK-test-device";
    connectParams.pHostURL = HostAddress;
    connectParams.port = port;
    connectParams.isWillMsgPresent = false;
    connectParams.pRootCALocation = AWS_IOT_ROOT_CA_FILENAME;
    connectParams.pDeviceCertLocation = AWS_IOT_CERTIFICATE_FILENAME;
    connectParams.pDevicePrivateKeyLocation = AWS_IOT_PRIVATE_KEY_FILENAME;
    connectParams.mqttCommandTimeout_ms = 2000;
    connectParams.tlsHandshakeTimeout_ms = 5000;
    connectParams.isSSLHostnameVerify = true;
    connectParams.disconnectHandler = disconnectCallbackHandler;

    INFO("Connecting...");
    rc = aws_iot_mqtt_connect(&connectParams);
    if (NONE_ERROR != rc) {
        ERROR("Error(%d) connecting to %s:%d", rc, connectParams.pHostURL,
                connectParams.port);
    }

    MQTTSubscribeParams subParams = MQTTSubscribeParamsDefault;
    subParams.mHandler = MQTTcallbackHandler;
    subParams.pTopic = "sdkTest/sub";
    subParams.qos = QOS_0;

    if (NONE_ERROR == rc) {
        INFO("Subscribing...");
        rc = aws_iot_mqtt_subscribe(&subParams);
        if (NONE_ERROR != rc) {
            ERROR("Error subscribing");
        }
    }

    MQTTMessageParams Msg = MQTTMessageParamsDefault;
    Msg.qos = QOS_0;
    char cPayload[100];
    sprintf(cPayload, "%s : %d ", "hello from SDK", i);
    Msg.pPayload = (void *) cPayload;
    Msg.PayloadLen = strlen(cPayload) + 1;

    MQTTPublishParams Params = MQTTPublishParamsDefault;
    Params.pTopic = "sdkTest/sub";
    Params.MessageParams = Msg;

    if(publishCount != 0){
       infinitePublishFlag = false;
    }

    while (NONE_ERROR == rc && (publishCount > 0 || infinitePublishFlag)) {
        rc = aws_iot_mqtt_yield(100);
        INFO("-->sleep");
        Task_sleep(1000);
        sprintf(cPayload, "%s : %d ", "hello from SDK", i++);
        rc = aws_iot_mqtt_publish(&Params);
        if(publishCount > 0){
           publishCount--;
        }
    }

    if (NONE_ERROR != rc){
        ERROR("An error occurred in the loop. Error code = %d\n", rc);
    }
    else {
        INFO("Publish done\n");
    }
}
