/*
 * Copyright 2015 Texas Instruments Incorporated. All Rights Reserved.
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
#include "network_interface.h"
#include "aws_iot_error.h"

#include <string.h>
#include <ti/net/socket.h>
#include <ti/net/http/sssl.h>
#include <simplelink.h>

int iot_tls_init(Network *pNetwork)
{
    if (pNetwork == NULL) {
        return (NULL_VALUE_ERROR);
    }

    pNetwork->my_socket = 0;
    pNetwork->mqttread = iot_tls_read;
    pNetwork->mqttwrite = iot_tls_write;
    pNetwork->disconnect = iot_tls_disconnect;

    return (NONE_ERROR);
}

int iot_tls_connect(Network *pNetwork, TLSConnectParams TLSParams)
{
    IoT_Error_t ret = TCP_CONNECT_ERROR;
    unsigned long ip;
    int skt;
    struct sockaddr_in address;
    Ssock_Handle ssock;
    SlSockSecureMethod method;
    SlSockSecureMask mask = {0};

    if (pNetwork == NULL) {
        return (NULL_VALUE_ERROR);
    }

    method.secureMethod = SL_SO_SEC_METHOD_TLSV1_2;

    SSSl_setContext(&method, &mask, TLSParams.pRootCALocation,
            TLSParams.pDevicePrivateKeyLocation,
            TLSParams.pDeviceCertLocation, NULL);

    if (gethostbyname((signed char *)TLSParams.pDestinationURL,
            strlen(TLSParams.pDestinationURL), &ip, AF_INET) == 0) {
        address.sin_family = AF_INET;
        address.sin_port = htons(TLSParams.DestinationPort);
        address.sin_addr.s_addr = htonl(ip);

        skt = socket(AF_INET, SOCK_STREAM, SL_SEC_SOCKET);
        if (skt >= 0) {
            ssock = Ssock_create(skt);

            if (SSSl_startSecure(ssock) == 0) {
                if (connect(skt, (struct sockaddr  *)&address,
                        sizeof(address)) >= 0) {
                    pNetwork->my_socket = (int)ssock;
                    ret = NONE_ERROR;
                }
                else {
                    ret = SSL_CONNECT_ERROR;
                }
            }

            if (ret != NONE_ERROR) {
                Ssock_delete(&ssock);
                close(skt);
            }
        }
    }

    return (ret);
}

int iot_tls_write(Network *pNetwork, unsigned char *pMsg, int len,
            int timeout_ms)
{
    Ssock_Handle ssock = NULL;
    int bytes = 0;

    if (pNetwork == NULL || pMsg == NULL || pNetwork->my_socket == 0
            || timeout_ms == 0) {
        return (NULL_VALUE_ERROR);
    }

    ssock = (Ssock_Handle)pNetwork->my_socket;

    bytes = Ssock_send(ssock, pMsg, len, 0);
    if (bytes >= 0) {
       return (bytes);
    }

    return (SSL_WRITE_ERROR);
}

int iot_tls_read(Network *pNetwork, unsigned char *pMsg, int len,
        int timeout_ms)
{
    int bytes = 0;
    int skt;
    struct timeval tv;
    Ssock_Handle ssock = NULL;

    if (pNetwork == NULL || pMsg == NULL || pNetwork->my_socket == 0
            || timeout_ms == 0) {
        return (NULL_VALUE_ERROR);
    }

    ssock = (Ssock_Handle)pNetwork->my_socket;

    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;

    skt = Ssock_getSocket(ssock);

    if (setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
            sizeof(struct timeval)) == 0) {
        bytes = Ssock_recv(ssock, pMsg, len, 0);
        if (bytes >= 0) {
            return (bytes);
        }
    }

    return (SSL_READ_ERROR);
}

void iot_tls_disconnect(Network *pNetwork)
{
    int skt;
    Ssock_Handle ssock = NULL;

    if (pNetwork == NULL || pNetwork->my_socket == 0) {
        return;
    }

    ssock = (Ssock_Handle)pNetwork->my_socket;
    skt = Ssock_getSocket(ssock);
    Ssock_delete(&ssock);
    close(skt);
}

int iot_tls_destroy(Network *pNetwork)
{
    if (pNetwork == NULL) {
        return (NULL_VALUE_ERROR);
    }

    pNetwork->my_socket = 0;
    pNetwork->mqttread = NULL;
    pNetwork->mqttwrite = NULL;
    pNetwork->disconnect = NULL;

    return (NONE_ERROR);
}
