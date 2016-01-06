/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== certflasher.c ========
 */
#include <stdlib.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/drivers/GPIO.h>

#include <simplelink.h>

#include "Board.h"

/* USER STEP: Fill in the ca.pem */
const char ca_pem[] = "[ca certificate string]";

/* USER STEP: Fill in the client_cert.pem */
const char client_cert_pem[] = "[client certificate string]";

/* USER STEP: Fill in the client_key.pem */
const char client_key_pem[] ="[client key string]";

/* USER STEP: Fill in the ca certificate DER filepath on serial flash */
const char ca_cert_path[] = "/cert/cacert.der";

/* USER STEP: Fill in the client certificate DER filepath on serial flash*/
const char client_cert_path[] = "/cert/clientcert.der";

/* USER STEP: Fill in the client key DER filepath on serial flash*/
const char client_key_path[] = "/cert/clientkey.der";

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pArgs)
{
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pArgs)
{
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pArgs)
{
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpServerEvent,
        SlHttpServerResponse_t *pHttpServerResponse)
{
}

/*
 *  ======== base64Decode ========
 *  Returns the decoded value
 */
unsigned char base64Decode(unsigned char ch)
{
    unsigned char ret = 0;
    if (ch >= 'A' && ch <= 'Z') {
        ret = ch - 'A';
    }
    else if (ch >= 'a' && ch <= 'z') {
        ret = ch - 'a' + 26;
    }
    else if (ch >= '0' && ch <= '9') {
        ret = ch - '0' + 52;
    }
    else if (ch == '+') {
        ret = 62;
    }
    else if (ch == '/') {
        ret = 63;
    }

    return (ret);
}

/*
 *  ======== convertPemToDer ========
 *  Returns the length of the der output buffer or -1 on failure
 */
int convertPemToDer(const char *pem, int plen, char** der)
{
    int value = 0;
    int i;
    int j;
    int dlen = 0;
    int padZero = 0;
    char *derPtr = NULL;

    for (i = plen; pem[i - 1] == '='; i--) {
        padZero++;
    }

    /* Base64 decode: 4 characters to 3 bytes */
    dlen = (plen * 3) / 4;
    derPtr = (char *) malloc (sizeof(char) * dlen);
    if (derPtr == NULL) {
        return (-1);
    }

    for (i = 0, j = 0; (i + 3) < plen && (j + 2) < dlen; i += 4, j += 3) {
        value = (base64Decode(pem[i]) << 18)
                + (base64Decode(pem[i + 1]) <<  12)
                + (base64Decode(pem[i + 2]) << 6)
                + base64Decode(pem[i + 3]);

        derPtr[j]     = (value >> 16) & 0xFF;
        derPtr[j + 1] = (value >> 8) & 0xFF;
        derPtr[j + 2] = value & 0xFF;
    }

    *der = derPtr;

    return (dlen - padZero);
}

/*
 *  ======== flashFile ========
 *  Returns 0 on success or  -1 on failure
 */
int flashFile(const char *path, const unsigned char *buffer, int len)
{
    long ret;
    long fileHandle;

    if (!path || !buffer) {
        System_printf("flashfile: Error invalid input params\n");
        ret = -1;
        goto flashdone;
    }

    ret = sl_FsOpen((unsigned char *)path,
            FS_MODE_OPEN_CREATE(len,  _FS_FILE_PUBLIC_WRITE), NULL,
            &fileHandle);
    if (ret < 0) {
        System_printf("flashfile: Error couldn't open the file %s"
                ", error code = %d\n", path, ret);
        ret = -1;
        goto flashdone;
    }

    ret = sl_FsWrite(fileHandle, 0, (unsigned char *)buffer, len);
    if (ret < 0) {
        sl_FsClose(fileHandle, NULL, NULL, 0);

        System_printf("flashfile: Error couldn't write to the file %s"
                ", error code = %d\n", path, ret);
        ret = -1;
        goto flashdone;
    }

    ret = sl_FsClose(fileHandle, NULL, NULL, 0);
    if (ret < 0) {
        System_printf("flashfile: Error couldn't close to the file %s"
                ", error code = %d\n", path, ret);
        ret = -1;
        goto flashdone;
    }

    ret = 0;

flashdone:
    return (ret);
}

/*
 *  ======== cc3200CertFlashTask ========
 */
void cc3200CertFlashTask(unsigned int arg0, unsigned int arg1)
{
    char *der;
    int dlen;

    if (sl_Start(0, 0, 0) < 0) {
        System_abort("cc3200CertFlashTask: Error initializing SimpleLink Wi-Fi"
                " stack\n");
    }

    System_printf("Flashing ca certificate file ...\n");

    der = NULL;
    dlen = convertPemToDer(ca_pem, strlen(ca_pem), &der);
    if ((dlen < 0)
            || (flashFile(ca_cert_path, (unsigned char *)der, dlen) == -1)) {
        System_abort("cc3200CertFlashTask: Error flashing ca certificate "
                "file\n");
    }
    free(der);

    System_printf("Flashing client certificate file ...\n");

    der = NULL;
    dlen = convertPemToDer(client_cert_pem, strlen(client_cert_pem), &der);
    if ((dlen < 0)
            || (flashFile(client_cert_path, (unsigned char *)der, dlen)
            == -1)) {
        System_abort("cc3200CertFlashTask: Error flashing client certificate"
                " file\n");
    }
    free(der);

    System_printf("Flashing client key file ...\n");

    der = NULL;
    dlen = convertPemToDer(client_key_pem, strlen(client_key_pem), &der);
    if ((dlen < 0)
            || (flashFile(client_key_path, (unsigned char *)der, dlen) == -1)) {
        System_abort("cc3200CertFlashTask: Error flashing client key file\n");
    }
    free(der);

    System_printf("done.\n");
    System_flush();
}

/*
 *  ======== main ========
 */
int main(int argc, char *argv[])
{
    Task_Handle taskHandle;
    Task_Params taskParams;

    Board_initGeneral();
    Board_initGPIO();

    GPIO_write(Board_LED0, Board_LED_ON);

    VStartSimpleLinkSpawnTask(3);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 4096;

    taskHandle = Task_create((Task_FuncPtr)cc3200CertFlashTask, &taskParams,
            NULL);
    if (taskHandle == NULL) {
        System_abort("main: failed to create a Task!\n");
    }

    BIOS_start();
}
