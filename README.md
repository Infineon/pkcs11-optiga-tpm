# Introduction

This document explains how an OPTIGA™ TPM SLx 9670 TPM2.0 can be integrated into a Raspberry Pi® to create a TPM-based PKCS #11 cryptographic token.

PKCS #11 is a Public-Key Cryptography Standard that defines a standard platform-independent API to access cryptographic services from tokens, such as hardware security modules (HSM) and smart cards. This document provides guidance on how to setup a TPM-based token on a Raspberry Pi®.

The document contains the guide to utilize either esysdb (SQLite) or FAPI as PKCS #11 backend.

# Prerequisites

Hardware prerequisites:
- [Raspberry Pi® 4](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/)
- [IRIDIUM9670 TPM2.0](https://www.infineon.com/cms/en/product/evaluation-boards/iridium9670-tpm2.0-linux/)\
  <img src="https://github.com/Infineon/pkcs11-optiga-tpm/raw/main/media/IRIDIUM9670-TPM2.png" width="30%">

# Getting Started

For detailed setup and information, please find the Application Note at [link](https://github.com/Infineon/pkcs11-optiga-tpm/raw/main/documents/tpm-appnote-pkcs11.pdf).

# License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.