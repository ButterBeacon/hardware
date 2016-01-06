# CC3200 Setup

This file contains the following information:
- How to build the AWS SDK and the samples for the TI CC3200 launchpad.
- How to run the samples.

THIS IS PRE-RELEASE SOFTWARE. CONTENTS AND DIRECTORY STRUCTURE ARE SUBJECT TO CHANGE.

## Table of Contents
- [Prerequisites](#Prerequisites)
- [Build the sample applications](#Build-SAMPLE)
- [Obtain certificate files](#Obtain-CERTS)
- [Build the certificate flasher tool](#Build-TOOL)
- [Setting up Code Composer Studio before running the samples](#Setup-CCS)
- [Running the certificate flasher tool](#Run-TOOL)
- [Running a sample](#Run-SAMPLE)

<a name="Prerequisites"></a>
## Prerequisites
Required hardware: [CC3200 Launchpad](http://www.ti.com/tool/cc3200-launchxl) with the latest firmware/service pack.

This procedure assumes you have already installed the AWS SDK on your development machine. These instructions refer to the folder that contains the AWS SDK on your local machine as `<AWS_INSTALL_DIR>`.

While not strictly required, we recommend that you install the following tools from TI in the same directory and that you use directory names without any whitespace. This documentation assumes that you install everything in a directory named `C:/ti`.

- Install [Code Composer Studio v6.1.1](http://www.ti.com/tool/ccstudio?keyMatch=code%20composer%20studio) or above

- Install [CC3200 SDK 1.1.0](http://www.ti.com/tool/cc3200sdk). Linux users can use a tool such as [Wine](https://www.winehq.org) to run the SDK installer.

- Install [TI-RTOS SDK for SimpleLink 2.14.01.20](http://downloads.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/tirtos/index.html) or above

- Install [the NS package](http://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/ns/ns_1_10_00_03.zip)

<a name="Build-SAMPLE"></a>
## Build the sample applications
1. Edit the `products.mak` file in `<AWS_INSTALL_DIR>/` using your favorite text editor. The variables `XDC_INSTALL_DIR, TIRTOS_INSTALL_DIR, BIOS_INSTALL_DIR, UIA_INSTALL_DIR, CC3200SDK_INSTALL_DIR` and `NS_INSTALL_DIR` must point to the locations where you installed these products. The variable `TI_ARM_CODEGEN_INSTALL_DIR` should point to the installation location of the TI ARM compiler in your CCS installation. After modification, these variable definitions should look similar to the following if you are working in Windows. (Windows users: note the use of "/" in the path).

    ```
    XDC_INSTALL_DIR ?= C:/ti/xdctools_3_31_01_33_core
    TIRTOS_INSTALL_DIR ?= C:/ti/tirtos_simplelink_2_14_01_20
    BIOS_INSTALL_DIR   ?= $(TIRTOS_INSTALL_DIR)/products/bios_6_42_02_29
    UIA_INSTALL_DIR    ?= $(TIRTOS_INSTALL_DIR)/products/uia_2_00_02_39
    NS_INSTALL_DIR     ?= C:/ti/ns_1_10_00_03
    CC3200SDK_INSTALL_DIR ?= C:/ti/CC3200SDK_1.1.0/cc3200-sdk
    TI_ARM_CODEGEN_INSTALL_DIR ?= C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5
    ```

2. If you are using Windows, open a Windows command prompt. If you are using Linux, open a terminal window.

Before building a specific application, complete the following steps (we are referring to the `subscribe_publish_sample` application here, but the same procedure applies to other samples as well):

1. Open the `aws_iot_config.h` file from the directory `<AWS_INSTALL_DIR>/sample_apps/subscribe_publish_sample/` in a text editor and update the value of the "AWS_IOT_MQTT_HOST" variable with the address to the AWS Icebreaker endpoint.

2. Update the value of the "AWS_IOT_MQTT_CLIENT_ID" and "AWS_IOT_MY_THING_NAME" as per the AWS Developer Guide

3. Verify the file names in the variables "AWS_IOT_ROOT_CA_FILENAME", "AWS_IOT_CERTIFICATE_FILENAME", and "AWS_IOT_PRIVATE_KEY_FILENAME" match the ones in `<AWS_INSTALL_DIR>/certs/platform_tirtos/cc3200/certflasher.c`. If you are using the default paths in certflasher.c, no change should be required.

    ```
    #define AWS_IOT_ROOT_CA_FILENAME       "/cert/cacert.der" ///< Root CA file name
    #define AWS_IOT_CERTIFICATE_FILENAME   "/cert/clientcert.der" ///< device signed certificate file name
    #define AWS_IOT_PRIVATE_KEY_FILENAME   "/cert/clientkey.der" ///< Device private key filename
    ```

4. From the directory `<AWS_INSTALL_DIR>/sample_apps/subscribe_publish_sample/platform_tirtos/cc3200/`, Open the file `main.c`. Search for "USER STEP" and update the current date-time macros.

5. Open the file `wificonfig.h`. Search for "USER STEP" and update the WIFI SSID and SECURITY_KEY macros.

On the command line, enter the following commands to build the application:

    ```
    cd <AWS_INSTALL_DIR>/sample_apps/subscribe_publish_sample/platform_tirtos/cc3200
    C:/ti/xdctools_3_31_01_33_core/gmake all
    ```

<a name="Obtain-CERTS"></a>
## Obtain certificate files
Certificate files used by the samples need to be obtained from AWS using this procedure:

1. Create a client certificate and private key, and attach a policy using the instructions in the section "Securing Communication Between a Thing and AWS Iot" in the Icebreaker Developer Guide.

2. Obtain the root CA certificate as specified in the section “Verify MQTT Subscribe and Publish” in the Icebreaker Developer Guide.

<a name="Build-TOOL"></a>
## Build the certificate flasher tool
Before building the tool, complete the following:

1. Open the file `certflasher.c` from the directory `<AWS_INSTALL_DIR>/certs/platform_tirtos/cc3200/`.

2. Search for "USER STEP" and update the CA root certificate string, the client certificate string, and the client (private) key string. These should be extracted from certificate (.pem) files, or from the JSON data if you used the command line CLI tool in the previous section (remember to remove the newline '\n' characters in the JSON data). A typical string would be of this format:

    ```
    const char ca_pem[] =
    "JQQGEwJVUzEQMA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96ZW1hbjERMA8G"
    "A1UECgwIU2F3dG9vdGgxEzARBgNVBAsMCkNvbnN1bHRpbmcxGDAWBlNVBAMMD3d3"
    "dy53b2xmc3NsLmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmc3NsLmNvbTAe"
    "Fw0xNTA1MDcxODIxMDFaFw0xOGAxMzExODIxMDFaMIGUMQswCQYDVQQGEwJVUzEQ"
    "MA4GA1UECAwHTW9udGFuYTEQMA4GA1UEBwwHQm96ZW1hbjERMA8GA1UECgwIU2F3"
    "dG9vdGgxEzARBgNVBAsMCkNvbnN1bHRpbmcxGDAWBgNVBAMMD3d3dy53b2xmc3Ns"
    "LmNvbTEfMB0GCSqGSIb3DQEJARYQaW5mb0B3b2xmy3NsLmNvbTCCASIwDQYJKoZI"
    "hvcNAQEBBQADggEPADCCAQoCggEBAL8Myi0Ush6EQlvNOB9K8k11EPG2NZ/fyn0D"
    "mNOs3gNm7irx2LB9bgdUCxCYIU2AyxIg58xP3kV9yXJ3MurKkLtpUhADL6jzlcXx";
    ```

On the command line, enter the following commands to build the application:
    ```
    cd <AWS_INSTALL_DIR>/certs/platform_tirtos/cc3200
    C:/ti/xdctools_3_31_01_33_core/gmake all
    ```

<a name="Setup-CCS"></a>
## Setting up Code Composer Studio before running the samples
1. Plug the CC3200 Launchpad into a USB port on your PC

2. Open a serial session to the appropriate COM port with the following settings:

    ```
    Baudrate:     9600
    Data bits:       8
    Stop bits:       1
    Parity:       None
    Flow Control: None
    ```

3. Open Code Composer Studio.

4. In Code Composer Studio, open the CCS Debug Perspective - Windows menu -> Open Perspective -> CCS Debug

5. Open the Target Configurations View - Windows menu -> Show View -> Target Configurations

6. Right-click on User Defined. Select New Target Configuration.

7. Use `CC3200.ccxml` as "File name". Hit Finish.

8. In the Basic window, select "Stellaris In-Circuit Debug Interface" as the "Connection", and check the box next to "CC3200" in "Board or Device". Hit Save.

9. Right-click "CC3200.ccxml" in the Target Configurations View. Hit Launch Selected Configuration.

10. Under the Debug View, right-click on "Stellaris In-Circuit Debug Interface_0/Cortex_M4_0". Select "Connect Target".

<a name="Run-TOOL"></a>
## Running the certificate flasher tool
All samples rely on a set of certificates from AWS. As a result, the certificates need to be stored once into flash memory prior to running the samples. To flash the certificates, simply run the flasher tool you have previously [built](#Build-TOOL) using this procedure:

1. Select Run menu -> Load -> Load Program..., and browse to the file `certflasher.out` in `<AWS_INSTALL_DIR>/certs/platform_tirtos/cc3200/`. Hit OK. This would load the program onto the board.

2. Run the application by pressing F8. The output in the CCS Console looks as follows:

    ```
    Flashing ca certificate file ...
    Flashing client certificate file ...
    Flashing client key file ...
    done.
    ```

3. Hit Alt-F8 (Suspend) to halt the CPU.

<a name="Run-SAMPLE"></a>
## Running a sample
1. Select Run menu -> Load -> Load Program..., and browse to the file `subscribe_publish_sample.out` in `<AWS_INSTALL_DIR>/sample_apps/subscribe_publish_sample/platform_tirtos/cc3200`. Hit OK. This would load the program onto the board. (The same procedure applies to other samples by substituting `subscribe_publish_sample`)

2. Run the application by pressing F8. Output would appear in your serial terminal session:

    ```
    CC3200 has connected to AP and acquired an IP address.
    IP Address: 192.168.1.7

    AWS IoT SDK Version 0.3.0-BETA

    Connecting...
    Subscribing...
    -->sleep
    Subscribe callback
    sdkTest/sub     hello from SDK : 0
    -->sleep
    Subscribe callback
    sdkTest/sub     hello from SDK : 1
    -->sleep
    ```
