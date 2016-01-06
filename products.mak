# Copyright 2015-2016 Texas Instruments Incorporated. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
# http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#
#
#  ======== products.mak ========
#

##################### All platforms ####################

XDC_INSTALL_DIR       = /path/to/xdctools_installation

##################### TI-RTOS ####################

TIRTOS_INSTALL_DIR    = /path/to/tirtos_installation
BIOS_INSTALL_DIR      = $(TIRTOS_INSTALL_DIR)/products/<bios_dir>
UIA_INSTALL_DIR       = $(TIRTOS_INSTALL_DIR)/products/<uia_dir>
NS_INSTALL_DIR        = /path/to/ns_installation

##################### TI-RTOS (CC3200) ####################

CC3200SDK_VERSION     = 1.1.0
CCWARE_INSTALL_DIR    = $(TIRTOS_INSTALL_DIR)/products/CC3200_driverlib_1.1.0
CC3200SDK_INSTALL_DIR = /path/to/cc3200-sdk

# Leave assignment empty to disable this toolchain
TI_ARM_CODEGEN_INSTALL_DIR = /path/to/TI_ARM_compiler
