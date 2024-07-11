[![Github actions](https://github.com/infineon/pkcs11-optiga-tpm/actions/workflows/main.yml/badge.svg)](https://github.com/infineon/pkcs11-optiga-tpm/actions)

# Introduction

This document outlines the process of integrating an OPTIGA™ TPM SLx 967x TPM2.0 to establish a TPM2-based PKCS #11 cryptographic token.

PKCS #11, a Public-Key Cryptography Standard, establishes a standardized, platform-independent API for accessing cryptographic services from tokens, including hardware security modules (HSMs) and smart cards. This guide provides instructions for setting up a TPM2-based token.

---

# Table of Contents

- **[Prerequisites](#prerequisites)**
- **[Preparing the Environment](#preparing-the-environment)**
    - **[One-Time Setup](#one-time-setup)**
    - **[Recurring Setup](#recurring-setup)**
- **[Backend Initialization](#backend-initialization)**
    - **[ESYSDB (SQLite3) Backend](#esysdb-sqlite3-backend)**
    - **[FAPI Backend](#fapi-backend)**
- **[PKCS #11 Token Initialization](#pkcs-11-token-initialization)**
    - **[pkcs11-tool from OpenSC](#pkcs11-tool-from-opensc)**
    - **[tpm2_ptool from tpm2-pkcs11 (ESYSDB)](#tpm2_ptool-from-tpm2-pkcs11-esysdb)**
        - **[Default Primary Key](#default-primary-key)**
        - **[Custom Primary Key (Storage)](#custom-primary-key-storage)**
        - **[Custom Primary Key (Platform)](#custom-primary-key-platform)**
        - **[Link Existing Ordinary Keys (Storage)](#link-existing-ordinary-keys-storage)**
        - **[Link Existing Ordinary Keys (Platform)](#link-existing-ordinary-keys-platform)**
        - **[Import 3rd Party Keys and Certificates](#import-3rd-party-keys-and-certificates)**
    - **[p11tool from GnuTLS](#p11tool-from-gnutls)**
    - **[key_import from tpm2-pkcs11](#key_import-from-tpm2-pkcs11)**
- **[Tools for Accessing PKCS #11 Tokens](#tools-for-accessing-pkcs-11-tokens)**
    - **[pkcs11-tool from OpenSC](#pkcs11-tool-from-opensc-1)**
    - **[libp11 from OpenSC (OpenSSL)](#libp11-from-opensc-openssl)**
    - **[p11-kit](#p11-kit)**
    - **[p11tool from GnuTLS](#p11tool-from-gnutls-1)**
    - **[NSS Tools](#nss-tools)**
- **[Housekeeping](#housekeeping)**
    - **[Housekeeping (ESYSDB)](#housekeeping-esysdb)**
    - **[Housekeeping (FAPI)](#housekeeping-fapi)**
- **[GitHub Actions](#github-actions)**
- **[License](#license)**

---

# Prerequisites

The integration guide has been CI tested for compatibility with the following platform and operating system. Please refer to section [GitHub Actions](#github-actions):

- Platform: x86_64
- Operating System: Ubuntu (22.04)

---

# Preparing the Environment
<!--section:setup-pt1-->

## One-Time Setup

Updates the package lists:
```all
$ sudo apt update
```

Install generic packages:
```all
$ sudo apt -y install autoconf-archive libcmocka0 libcmocka-dev procps \
    iproute2 build-essential git pkg-config gcc libtool automake libssl-dev \
    uthash-dev autoconf doxygen libjson-c-dev libini-config-dev \
    libcurl4-openssl-dev uuid-dev pandoc acl libglib2.0-dev xxd \
    libsqlite3-dev libyaml-dev python3-pip libp11-kit-dev dbus opensc \
    meson libtasn1-bin libnss3-tools
```

Install Python packages:
```all
$ python3 -m pip install --upgrade pip==24.0
$ python3 -m pip install pyasn1_modules==0.2.8 cryptography==38.0.4 \
    pyyaml==6.0.1
```

Install tpm2-tss:
```all
$ git clone https://github.com/tpm2-software/tpm2-tss ~/tpm2-tss
$ cd ~/tpm2-tss
$ git checkout 4.0.1
$ ./bootstrap
$ ./configure
$ make -j
$ sudo make install
$ sudo ldconfig
```

Install tpm2-tools:
```all
$ git clone https://github.com/tpm2-software/tpm2-tools ~/tpm2-tools
$ cd ~/tpm2-tools
$ git checkout 5.6
$ ./bootstrap
$ ./configure
$ make -j
$ sudo make install
$ sudo ldconfig
```

Install tpm2-abrmd:
```all
$ git clone https://github.com/tpm2-software/tpm2-abrmd ~/tpm2-abrmd
$ cd ~/tpm2-abrmd
$ git checkout 3.0.0
$ ./bootstrap
$ ./configure
$ make -j
$ sudo make install
$ sudo ldconfig
```

Install tpm2-pytss:
```all
$ git clone https://github.com/tpm2-software/tpm2-pytss ~/tpm2-pytss
$ cd ~/tpm2-pytss
$ git checkout 2.2.1
$ python3 -m pip install -e .
```

Download the tpm2-pkcs11:
```all
$ git clone https://github.com/tpm2-software/tpm2-pkcs11 ~/tpm2-pkcs11
$ cd ~/tpm2-pkcs11
$ git checkout 50a636bdb19220bf926cf6664e3084a315fe4480
```

<!--section-end-->
<!--section:setup-esysdb-install-->

Install tpm2-pkcs11, two options are available:
1. ESYSDB (SQLite3) as the PKCS #11 backend:
    > Disable the FAPI backend at build time by adding `--disable-fapi` to prevent unnecessary errors and warnings from the FAPI backend.
    > Optionally, disable FAPI error logging by setting `export TSS2_LOG=fapi+NONE`.
    ```all
    $ cd ~/tpm2-pkcs11
    $ git clean -fxd
    $ ./bootstrap
    $ ./configure --disable-fapi
    $ make -j
    $ sudo make install
    $ sudo ldconfig
    ```

<!--section-end-->
<!--section:setup-fapi-install-->

2. FAPI as the PKCS #11 backend:
    ```all
    $ cd ~/tpm2-pkcs11
    $ git clean -fxd
    $ ./bootstrap
    $ ./configure
    $ make -j
    $ sudo make install
    $ sudo ldconfig
    ```

<!--section-end-->
<!--section:setup-pt2-->

Install libtpms-based TPM emulator:
```all
# Install dependencies
$ sudo apt install -y dh-autoreconf libtasn1-6-dev net-tools \
    libgnutls28-dev expect gawk socat libfuse-dev libseccomp-dev \
    make libjson-glib-dev gnutls-bin

# Install libtpms-devel
$ git clone https://github.com/stefanberger/libtpms ~/libtpms
$ cd ~/libtpms
$ git checkout v0.9.6
$ ./autogen.sh --with-tpm2 --with-openssl
$ make -j
$ sudo make install
$ sudo ldconfig

# Install Libtpms-based TPM emulator
$ git clone https://github.com/stefanberger/swtpm ~/swtpm
$ cd ~/swtpm
$ git checkout v0.8.1
$ ./autogen.sh --with-openssl --prefix=/usr
$ make -j
$ sudo make install
$ sudo ldconfig
```

Install the latest version of p11-kit to access a broader range of utilities. If installed via `apt install p11-kit`, only a limited set of subcommands is available:
> It is important to set the installation path correctly for p11-kit to be able to locate the PKCS #11 module configuration files (e.g., `tpm2_pkcs11.module` from tpm2-pkcs11), which are located in `/usr/share/p11-kit/modules/` in this setup.
> The PKCS #11 module configuration file is a file that contains a description of the PKCS #11 module and the path to the module (e.g., `libtpm2_pkcs11.so` from tpm2-pkcs11).
```all
$ git clone https://github.com/p11-glue/p11-kit -b 0.25.3 --depth=1 ~/p11-kit
$ cd ~/p11-kit
$ meson setup _build
$ meson configure _build -Dprefix=/usr
$ meson compile -C _build
$ meson install -C _build
$ sudo ldconfig
```

Install the later version of [GnuTLS](https://www.gnutls.org/download.html) from the source:
```all
# Install dependencies
$ sudo apt install -y wget

# Download the source
$ cd ~
$ wget --no-verbose https://www.gnupg.org/ftp/gcrypt/gnutls/v3.8/gnutls-3.8.3.tar.xz
$ tar -xf gnutls-3.8.3.tar.xz

# Build and install the project
$ cd ~/gnutls-3.8.3
$ ./configure --with-included-unistring --disable-doc --disable-tests
$ make -j
$ sudo make install
$ sudo ldconfig
```

<!--
Install OpenSC:
```
# Install dependencies
$ sudo apt install -y libpcsclite-dev

# Install OpenSC
$ git clone https://github.com/OpenSC/OpenSC ~/OpenSC
$ cd ~/OpenSC
$ git checkout 0.25.0
$ ./bootstrap
$ ./configure
$ make -j
$ sudo make install
$ sudo ldconfig
```
-->

<!--
Change workspace:
```all
$ cd /tmp
```
-->

## Recurring Setup

This guide utilizes the session dbus. While using the system dbus is possible, it is not included in this documentation.

Start a session dbus limited to the current login session:
```all
$ export DBUS_SESSION_BUS_ADDRESS=`dbus-daemon --session --print-address --fork`
```

Launch the libtpms-based TPM emulator:
```all
$ export SWTPM_PATH=$(mktemp -d)

# Create configuration files for swtpm_setup:
# - ~/.config/swtpm_setup.conf
# - ~/.config/swtpm-localca.conf
#   This file specifies the location of the CA keys and certificates:
#   - ~/.config/var/lib/swtpm-localca/*.pem
# - ~/.config/swtpm-localca.options
$ swtpm_setup --tpm2 --create-config-files overwrite,root

# Initialize the swtpm
$ swtpm_setup --tpm2 --config ~/.config/swtpm_setup.conf --tpm-state ${SWTPM_PATH} \
    --overwrite --create-ek-cert --create-platform-cert --write-ek-cert-files ${SWTPM_PATH}

# Launch the swtpm
$ swtpm socket --tpm2 --flags not-need-init --tpmstate dir=${SWTPM_PATH} \
    --server type=tcp,port=2321 --ctrl type=tcp,port=2322 &
$ sleep 5
```

Launch the TPM Resource Manager daemon; it facilitates communication via session dbus and acts as an intermediary to the TPM emulator:
```all
$ tpm2-abrmd --allow-root --session --tcti=swtpm:host=127.0.0.1,port=2321 &
$ sleep 5
```

Specifies the TCTI using an environment variable:
```all
$ export TPM2TOOLS_TCTI="tabrmd:bus_name=com.intel.tss2.Tabrmd,bus_type=session"
$ export TPM2_PKCS11_TCTI="tabrmd:bus_name=com.intel.tss2.Tabrmd,bus_type=session"
```

For convenience, use an alias to shorten the command:
```all
$ TPM2_P11_PATH=`pkg-config --variable=p11_module_path tpm2-pkcs11`
$ alias pkcs11-tool-tpm2="pkcs11-tool --module ${TPM2_P11_PATH}/libtpm2_pkcs11.so"
$ alias tpm2_ptool="${HOME}/tpm2-pkcs11/tools/tpm2_ptool/tpm2_ptool.py"
$ alias
```

A simple test to verify that communication with the TPM is in order:
```all
$ tpm2_getrandom --hex 16
```

<!--section-end-->

---

# Backend Initialization

To meet the requirements of the PKCS #11 interface, additional metadata needs to be saved to a data "store". The provider of this "store" is referred to as the backend of the PKCS #11 token, which includes the following backend options:

1. [ESYSDB (SQLite3)](#esysdb-sqlite3-backend)
2. [FAPI](#fapi-backend)

## ESYSDB (SQLite3) Backend
<!--section:backend-init-esysdb-->

Clear the TPM:
```all
$ tpm2_clear -c p
```

Set the token store location:
> If not specified, the default path is set to `$HOME/.tpm2_pkcs11/tpm2_pkcs11.sqlite3`.
```all
$ export TPM2_PKCS11_STORE=$(mktemp -d)
```

Leaving the `TPM2_PKCS11_BACKEND` environment variable unset or setting it to `esysdb` defaults to the SQLite3 backend:
```all
$ export TPM2_PKCS11_BACKEND="esysdb"
```

<!--
A workaround to prevent error "FAPI_METADATA_DIR: unbound variable" in test section "tools-pkcs11-tool".
```all
$ export FAPI_METADATA_DIR=$(mktemp -d)
```
-->

<!--section-end-->

## FAPI Backend
<!--section:backend-init-fapi-->

Set the FAPI metadata store location:
```all
$ export FAPI_METADATA_DIR=$(mktemp -d)
```

Initialize a TPM 2.0-based PKCS#11 Token with the TPM Feature API (FAPI) as backend. First, is to provisions a FAPI instance.

Update the FAPI configuration file at `/usr/local/etc/tpm2-tss/fapi-config.json`. The changes include:
- Move all working directories to `${FAPI_METADATA_DIR}`.
- The profile can be `P_RSA2048SHA256` or `P_ECCP256SHA256`. For better performance, `P_ECCP256SHA256` may be used.
- Additionally, updating the TCTI parameter allows switching between hardware or a simulated TPM. If using a TPM simulator, it is also possible to set it to `ek_cert_less`.
```all
$ rm /usr/local/etc/tpm2-tss/fapi-config.json
$ cat > /usr/local/etc/tpm2-tss/fapi-config.json << EOF
$ {
$     "profile_name": "P_RSA2048SHA256",
$     "profile_dir": "/usr/local/etc/tpm2-tss/fapi-profiles/",
$     "user_dir": "${FAPI_METADATA_DIR}/user/keystore/",
$     "system_dir": "${FAPI_METADATA_DIR}/system/keystore/",
$     "tcti": "tabrmd:bus_name=com.intel.tss2.Tabrmd,bus_type=session",
$     "ek_cert_less": "yes",
$     "system_pcrs" : [],
$     "log_dir" : "${FAPI_METADATA_DIR}/eventlog/"
$ }
$ EOF
$ cat /usr/local/etc/tpm2-tss/fapi-config.json
```

Clear the TPM:
```all
$ tpm2_clear -c p
```

Provision a FAPI instance. The Storage Root Key (SRK) will be created based on the profile located at `profile_dir/profile_name` and made persistent at handle 0x81000001 (as specified in the profile) without an authorization value. TPM metadata will be stored in the directory `system_dir`:
```all
$ tss2_provision
```

Set the `TPM2_PKCS11_BACKEND` environment variable to `fapi` for using the FAPI backend:
```all
$ export TPM2_PKCS11_BACKEND="fapi"
```

<!--section-end-->

---

# PKCS #11 Token Initialization

Initialize a TPM2-based PKCS #11 token. The options include:
1. **[pkcs11-tool from OpenSC](#pkcs11-tool-from-opensc)**
2. **[tpm2_ptool from tpm2-pkcs11 (ESYSDB)](#tpm2_ptool-from-tpm2-pkcs11-esysdb)**
3. **[p11tool from GnuTLS](#p11tool-from-gnutls)**

Late import of TPM persistent keys or externally stored keys into the TPM2-based PKCS #11 token using vendor-specific PKCS #11 attributes:
- **[key_import from tpm2-pkcs11](#key_import-from-tpm2-pkcs11)**

## pkcs11-tool from OpenSC
<!--section:token-init-pkcs11-tool-->

Initialize a TPM2-based PKCS #11 token using `pkcs11-tool`.
> If the backend is ESYSDB and the backend store is not found, this process automatically creates the `tpm2_pkcs11.sqlite3` store,
> generates a SRK, and persistently stores it at the handle 0x81000001.

Create a token:
```all
$ pkcs11-tool-tpm2 --slot-index 0 --init-token --label tpm2-token --so-pin sopin
$ pkcs11-tool-tpm2 --slot-index 0 --init-pin --so-pin sopin --login --pin userpin
```

List the token:
```all
$ pkcs11-tool-tpm2 --list-token-slots
```

<!--section-end-->

## tpm2_ptool from tpm2-pkcs11 (ESYSDB)

This section only works with the ESYSDB backend. The tool directly modifies the ESYSDB backend store to make changes or additions. Subsequently, these changes will be reflected in the PKCS #11 interface.

Initialize a TPM2-based PKCS #11 token using the Python-based `tpm2_ptool`. The options include automatically generating the SRK (Storage Root Key) as the default behavior or manually specifying the primary key:
1. [Default Primary Key](#default-primary-key)
2. [Custom Primary Key (Storage)](#custom-primary-key-storage)
2. [Custom Primary Key (Platform)](#custom-primary-key-platform)

Furthermore, `tpm2_ptool` enables the association of existing TPM externally stored keys (in Platform/Storage hierarchy) with the token:
- [Link Existing Ordinary Keys (Storage)](#link-existing-ordinary-keys-storage)
- [Link Existing Ordinary Keys (Platform)](#link-existing-ordinary-keys-platform)

For importing keys and certificates generated by third parties into the token using `tpm2_ptool`:
- [Import 3rd Party Keys and Certificates](#import-3rd-party-keys-and-certificates)

### Default Primary Key
<!--section:token-init-tpm2-ptool-default-primary-key-->

Create the backend store `tpm2_pkcs11.sqlite3`, generate a SRK, and persistently store it at handle 0x81000001:
> If the hierarchy is protected by an authentication value, use the `--hierarchy-auth` option to set this value.

> It is possible to set the authentication value of the primary key using the option `--primary-auth`.
```all
$ pid=$(tpm2_ptool init --path $TPM2_PKCS11_STORE | grep id | sed 's/id: //')
```

Create a token:
```all
$ tpm2_ptool addtoken --pid $pid --sopin sopin --userpin userpin \
    --label tpm2-token --path $TPM2_PKCS11_STORE
```

List the token:
```all
$ pkcs11-tool-tpm2 --list-token-slots
```

<!--section-end-->

### Custom Primary Key (Storage)
<!--section:token-init-tpm2-ptool-custom-primary-key-storage-->

Create the SRK:
```all
$ tpm2_createprimary -G ecc -c primary.ctx
$ tpm2_evictcontrol -c primary.ctx 0x81000001
```

Create the backend store `tpm2_pkcs11.sqlite3`, associating it with the specified primary handle:
> If the primary key is protected by an authentication value, use the `--primary-auth` option to set this value.
```all
$ export TPM2_PKCS11_STORE=$(mktemp -d)
$ pid=$(tpm2_ptool init --primary-handle 0x81000001 --path $TPM2_PKCS11_STORE | grep id | sed 's/id: //')
```

Create the token:
```all
$ tpm2_ptool addtoken --pid $pid --sopin sopin --userpin userpin \
    --label tpm2-token --path $TPM2_PKCS11_STORE
```

List the token:
```all
$ pkcs11-tool-tpm2 --list-token-slots
```

<!--section-end-->

### Custom Primary Key (Platform)
<!--section:token-init-tpm2-ptool-custom-primary-key-platform-->

Create the SRK:
```all
$ tpm2_createprimary -C p -G ecc -c primary.ctx
$ tpm2_evictcontrol -C p -c primary.ctx 0x81800001
```

Create the backend store `tpm2_pkcs11.sqlite3`, associating it with the specified primary handle:
> If the primary key is protected by an authentication value, use the `--primary-auth` option to set this value.
```all
$ export TPM2_PKCS11_STORE=$(mktemp -d)
$ pid=$(tpm2_ptool init --primary-handle 0x81800001 --path $TPM2_PKCS11_STORE | grep id | sed 's/id: //')
```

Create the token:
```all
$ tpm2_ptool addtoken --pid $pid --sopin sopin --userpin userpin \
    --label tpm2-token --path $TPM2_PKCS11_STORE
```

List the token:
```all
$ pkcs11-tool-tpm2 --list-token-slots
```

<!--section-end-->

### Link Existing Ordinary Keys (Storage)
<!--section:token-init-tpm2-ptool-link-keys-storage-->

This requires a token initialized with a primary key in the storage hierarchy.

Create the ordinary keys:
```all
$ tpm2_create -G rsa2048 -C 0x81000001 -u rsakey.pub -r rsakey.priv
$ tpm2_create -G ecc -C 0x81000001 -u ecckey.pub -r ecckey.priv
```

Link existing RSA and ECC keys to the token:
```all
$ tpm2_ptool link --label tpm2-token --id 0 --key-label linked-rsa-2048 \
    --userpin userpin --path $TPM2_PKCS11_STORE rsakey.pub rsakey.priv
$ tpm2_ptool link --label tpm2-token --id 1 --key-label linked-ecc-p256 \
    --userpin userpin --path $TPM2_PKCS11_STORE ecckey.pub ecckey.priv
```

Read the slot number:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
```

List the keys:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-objects --login --pin userpin
```

<!--section-end-->

### Link Existing Ordinary Keys (Platform)
<!--section:token-init-tpm2-ptool-link-keys-platform-->

This requires a token initialized with a primary key in the platform hierarchy.

Create the ordinary keys:
```all
$ tpm2_create -G rsa2048 -C 0x81800001 -u rsakey.pub -r rsakey.priv
$ tpm2_create -G ecc -C 0x81800001 -u ecckey.pub -r ecckey.priv
```

Link existing RSA and ECC keys to the token:
```all
$ tpm2_ptool link --label tpm2-token --id 0 --key-label linked-rsa-2048 \
    --userpin userpin --path $TPM2_PKCS11_STORE rsakey.pub rsakey.priv
$ tpm2_ptool link --label tpm2-token --id 1 --key-label linked-ecc-p256 \
    --userpin userpin --path $TPM2_PKCS11_STORE ecckey.pub ecckey.priv
```

Read the slot number:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
```

List the keys:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-objects --login --pin userpin
```

<!--section-end-->

### Import 3rd Party Keys and Certificates
<!--section:token-init-tpm2-ptool-import-keys-certs-->

Generate an RSA key and a self-signed CA certificate using OpenSSL:
```all
$ openssl req -x509 -nodes -days 3650 -newkey rsa:2048 \
    -keyout ca.key.pem -out ca.crt.pem \
    -subj "/C=DE/ST=Some-State/O=Some Inc./CN=dummy"
```

Import the key to the token:
```all
$ tpm2_ptool import --label tpm2-token --key-label imported-ca-key \
    --userpin userpin --algorithm rsa --privkey ca.key.pem
```

Add the certificate to the token:
```all
$ tpm2_ptool addcert --label tpm2-token --key-label imported-ca-key \
    --path $TPM2_PKCS11_STORE ca.crt.pem
```

Read the slot number:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
```

List the keys and certificates:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-objects --login --pin userpin
```

<!--section-end-->

## p11tool from GnuTLS
<!--section:token-init-p11tool-->

Initialize a TPM-based PKCS #11 token using `p11tool`.
> If the backend is ESYSDB and the backend store is not found, this process automatically creates the `tpm2_pkcs11.sqlite3` store,
> generates a SRK, and persistently stores it at the handle 0x81000001.

Get the token URL:
```all
$ TOKEN_URL=`p11tool --list-token-urls | head -n 1`
```

Create a token:
```all
$ p11tool --initialize --label tpm2-token --set-so-pin sopin "$TOKEN_URL"
```

Set user pin:
```all
$ p11tool --initialize-pin --so-login --set-pin userpin "pkcs11:token=tpm2-token;pin-value=sopin"
```

<!--section-end-->

## key_import from tpm2-pkcs11
<!--section:token-init-key-import-->

The `key_import` tool from tpm2-pkcs11 is a C program that utilizes PKCS #11 vendor-specific attributes to import TPM2 keys (as persistent handles or externally stored key objects, with or without auth value). This approach does not directly modify the backend store. Instead, it uses the PKCS #11 standard interface along with vendor-specific attributes to import the keys. The advantage of this approach is the reduced dependency on additional tooling needed to perform the key import operation, and it works well with either backend (ESYSDB or FAPI). For more examples, please refer to [KEY_IMPORT_TOOL.md](https://github.com/tpm2-software/tpm2-pkcs11/blob/master/docs/KEY_IMPORT_TOOL.md).

Create the ordinary keys that will be imported later:
```all
# Externally stored key objects

$ tpm2_create -G rsa2048 -C 0x81000001 -u rsakey.pub -r rsakey.priv
$ tpm2_create -G ecc -C 0x81000001 -p ecckeyauth -u ecckey.pub -r ecckey.priv

# Persistent keys

$ tpm2_create -G rsa2048 -C 0x81000001 -p rsakeyauth -u rsakey_tmp.pub -r rsakey_tmp.priv
$ tpm2_load -C 0x81000001 -u rsakey_tmp.pub -r rsakey_tmp.priv -c rsakey_tmp.ctx
$ tpm2_evictcontrol -C o -c rsakey_tmp.ctx 0x81000011

$ tpm2_create -G ecc -C 0x81000001 -u ecckey_tmp.pub -r ecckey_tmp.priv
$ tpm2_load -C 0x81000001 -u ecckey_tmp.pub -r ecckey_tmp.priv -c ecckey_tmp.ctx
$ tpm2_evictcontrol -C o -c ecckey_tmp.ctx 0x81000012
```

Before proceeding, ensure that PKCS #11 token is initialized.

Read the slot number using the `pkcs11_tool`. The slot numbers returned by the tool start from 0, while the slot numbers in the tpm2-pkcs11 library start from 1:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
$ TPM2_PKCS11_SLOT_INDEX=$((SLOT_INDEX + 1))
```

Import the externally stored key objects:
```all
$ key_import --slot-id $TPM2_PKCS11_SLOT_INDEX --user-pin userpin \
    --tcti "$TPM2TOOLS_TCTI" \
    --parent-persistent-handle 0x81000001 \
    --public rsakey.pub \
    --private rsakey.priv \
    --key-label imported-rsa-2048-obj

$ key_import --slot-id $TPM2_PKCS11_SLOT_INDEX --user-pin userpin \
    --tcti "$TPM2TOOLS_TCTI" \
    --parent-persistent-handle 0x81000001 \
    --public ecckey.pub \
    --private ecckey.priv \
    --key-auth ecckeyauth \
    --key-label imported-ecc-p256-obj
```

Import the persistent keys:
```all
$ key_import --slot-id $TPM2_PKCS11_SLOT_INDEX --user-pin userpin \
    --tcti "$TPM2TOOLS_TCTI" \
    --persistent-handle 0x81000011 \
    --key-auth rsakeyauth \
    --key-label imported-rsa-2048-persist

$ key_import --slot-id $TPM2_PKCS11_SLOT_INDEX --user-pin userpin \
    --tcti "$TPM2TOOLS_TCTI" \
    --persistent-handle 0x81000012 \
    --key-label imported-ecc-p256-persist
```

List the keys:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-objects --login --pin userpin | tee log2verify
$ cat log2verify | grep imported-rsa-2048-obj
$ cat log2verify | grep imported-ecc-p256-obj
$ cat log2verify | grep imported-rsa-2048-persist
$ cat log2verify | grep imported-ecc-p256-persist
$ rm log2verify
```

Read the public components:
```all
# key: imported-rsa-2048-obj

$ rsa_obj_id=$(echo -n imported-rsa-2048-obj | xxd -p)
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $rsa_obj_id --type pubkey \
    --read-object --output-file rsa_obj.pub.der
$ openssl rsa -inform DER -outform PEM -in rsa_obj.pub.der -pubin \
    -out rsa_obj.pub.pem
$ cat rsa_obj.pub.pem

# key: imported-ecc-p256-obj

$ ecc_obj_id=$(echo -n imported-ecc-p256-obj | xxd -p)
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $ecc_obj_id --type pubkey \
    --read-object --output-file ecc_obj.pub.der
$ openssl ec -inform DER -outform PEM -in ecc_obj.pub.der -pubin \
    -out ecc_obj.pub.pem
$ cat ecc_obj.pub.pem

# key: imported-rsa-2048-persist

$ rsa_persist_id=$(echo -n imported-rsa-2048-persist | xxd -p)
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $rsa_persist_id --type pubkey \
    --read-object --output-file rsa_persist.pub.der
$ openssl rsa -inform DER -outform PEM -in rsa_persist.pub.der -pubin \
    -out rsa_persist.pub.pem
$ cat rsa_persist.pub.pem

# key: imported-ecc-p256-persist

$ ecc_persist_id=$(echo -n imported-ecc-p256-persist | xxd -p)
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $ecc_persist_id --type pubkey \
    --read-object --output-file ecc_persist.pub.der
$ openssl ec -inform DER -outform PEM -in ecc_persist.pub.der -pubin \
    -out ecc_persist.pub.pem
$ cat ecc_persist.pub.pem
```

Create a file with random data:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --generate-random 32 \
    --output-file data.plain
```

Perform RSA encryption and decryption:
```all
# key: imported-rsa-2048-obj

$ openssl rsautl -encrypt -inkey rsa_obj.pub.pem -in data.plain \
        -pubin -out data.cipher
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $rsa_obj_id --decrypt \
    --mechanism RSA-PKCS --input-file data.cipher \
    --output-file data.decipher
$ diff data.plain data.decipher

# key: imported-rsa-2048-persist

$ openssl rsautl -encrypt -inkey rsa_persist.pub.pem -in data.plain \
        -pubin -out data.cipher
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin \
    --id $rsa_persist_id --decrypt \
    --mechanism RSA-PKCS --input-file data.cipher \
    --output-file data.decipher
$ diff data.plain data.decipher
```

Perform RSA sign and verification:
```all
# key: imported-rsa-2048-obj

$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id $rsa_obj_id \
    --login --pin userpin --sign \
    --mechanism SHA256-RSA-PKCS --input-file data.plain \
    --output-file data.rsa.sig
$ openssl dgst -sha256 -verify rsa_obj.pub.pem \
    -signature data.rsa.sig data.plain

# key: imported-rsa-2048-persist

$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id $rsa_persist_id \
    --login --pin userpin --sign \
    --mechanism SHA256-RSA-PKCS --input-file data.plain \
    --output-file data.rsa.sig
$ openssl dgst -sha256 -verify rsa_persist.pub.pem \
    -signature data.rsa.sig data.plain
```

Perform ECC sign and verification:
```all
# key: imported-ecc-p256-obj

$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id $ecc_obj_id \
    --login --pin userpin --sign \
    --mechanism ECDSA-SHA1 --signature-format openssl \
    --input-file data.plain --output-file data.ec.sig
$ openssl dgst -sha1 -verify ecc_obj.pub.pem \
    -signature data.ec.sig data.plain

# key: imported-ecc-p256-persist

$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id $ecc_persist_id \
    --login --pin userpin --sign \
    --mechanism ECDSA-SHA1 --signature-format openssl \
    --input-file data.plain --output-file data.ec.sig
$ openssl dgst -sha1 -verify ecc_persist.pub.pem \
    -signature data.ec.sig data.plain
```

<!--section-end-->

---

# Tools for Accessing PKCS #11 Tokens

Tools available for operations on a PKCS #11 token include:
- [pkcs11-tool from OpenSC](#pkcs11-tool-from-opensc-1)
- [libp11 from OpenSC (OpenSSL)](#libp11-from-opensc-openssl)
- [p11-kit](#p11-kit)
- [p11tool from GnuTLS](#p11tool-from-gnutls-1)
- [NSS Tools](#nss-tools)

## pkcs11-tool from OpenSC
<!--section:tools-pkcs11-tool-->

The pkcs11-tool is one of the command-line tools provided by [OpenSC](https://github.com/OpenSC/OpenSC). This section introduces the tool through a series of practical examples, demonstrating its capabilities and usage.

The pkcs11-tool is installed during the setup stage through the installation of the `opensc` package.

Check the version of the `opensc` package and its dependencies:
```all
$ apt show opensc
$ apt-cache depends opensc | grep Depends: | cut -d ":" -f2 | xargs dpkg -l
```

List available tokens:
```all
$ pkcs11-tool-tpm2 --list-token-slots
```

Read the slot number:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
```

Generate random bytes:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --generate-random 32 --output-file data.plain
```

Change the SO pin:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --change-pin --login --login-type so --so-pin sopin --new-pin sopin2
```

Restore the SO pin:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --change-pin --login --login-type so --so-pin sopin2 --new-pin sopin
```

Change the user pin using user credential (from "userpin" to "upin"):
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --change-pin --login --pin userpin --new-pin upin
```

Change the user pin using SO credential (from "upin" to "userpin"):
```all
# This is required for the FAPI backend; otherwise, the overwrite check will fail.
$ rm -rf ${FAPI_METADATA_DIR}/user/keystore/P_RSA2048SHA256/HS/SRK/tpm2-pkcs11-token-usr-00000001

$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --init-pin --login --login-type so --so-pin sopin --new-pin userpin
```

Display supported mechanisms:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-mechanisms
```

Create an RSA key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id 00 --label rsa2048 --login --pin userpin \
    --keypairgen --key-type RSA:2048
```

Create an ECC key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id 01 --label eccp256 --login --pin userpin \
    --keypairgen --usage-sign --key-type EC:secp256r1
```

List the created keys:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --list-objects --login --pin userpin
```

Read the public component of the RSA key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --id 00 --type pubkey \
    --read-object --output-file rsa.pub.der
$ openssl rsa -inform DER -outform PEM -in rsa.pub.der -pubin -out rsa.pub.pem
```

Read the public component of the ECC key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --id 01 --type pubkey \
    --read-object --output-file ecc.pub.der
$ openssl ec -inform DER -outform PEM -in ecc.pub.der -pubin -out ecc.pub.pem
```

Perform RSA encryption and decryption:
```all
$ openssl rsautl -encrypt -inkey rsa.pub.pem -in data.plain -pubin -out data.cipher
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --id 00 --decrypt \
    --mechanism RSA-PKCS --input-file data.cipher \
    --output-file data.decipher
$ diff data.plain data.decipher
```

Perform RSA signing and signature verification:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id 00 --login --pin userpin --sign \
    --mechanism SHA256-RSA-PKCS --input-file data.plain \
    --output-file data.rsa.sig
$ openssl dgst -sha256 -verify rsa.pub.pem -signature data.rsa.sig data.plain
```

Perform ECC signing and signature verification:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --id 01 --login --pin userpin --sign \
    --mechanism ECDSA-SHA1 --signature-format openssl \
    --input-file data.plain --output-file data.ecc.sig
$ openssl dgst -sha1 -verify ecc.pub.pem -signature data.ecc.sig data.plain
```

Destroy the RSA key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --delete-object --type privkey --id 00
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --delete-object --type pubkey --id 00
```

Destroy the ECC key:
```all
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --delete-object --type privkey --id 01
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --login --pin userpin --delete-object --type pubkey --id 01
```

<!--section-end-->

## libp11 from OpenSC (OpenSSL)

[libp11](https://github.com/OpenSC/libp11) is a PKCS #11 engine plugin for the OpenSSL library, provided by [OpenSC](https://github.com/OpenSC). Since the OpenSSL engine was deprecated with the release of OpenSSL 3, and OpenSSL now recommends using Providers instead of Engines, examples of using libp11 will not be covered here. Currently, OpenSC does not officially support a PKCS #11 provider, as discussed in this [issue](https://github.com/OpenSC/libp11/issues/432).

## p11-kit
<!--section:tools-p11-kit-->

The [p11-kit](https://github.com/p11-glue/p11-kit) project includes the libp11-kit library for loading, enumerating, and [managing](https://p11-glue.github.io/p11-glue/p11-kit/manual/sharing-managed.html) PKCS #11 modules and also comes bundled with a command-line tool. This section introduces the tool through a series of practical examples, showcasing its capabilities and usage.

The p11-kit project is built and installed during the setup stage.

Please refer to the p11-kit installation section to learn how p11-kit discovers PKCS #11 modules (e.g., `libtpm2_pkcs11.so` from tpm2-pkcs11).

List available tokens:
```all
$ p11-kit list-modules
```

Display a token information:
```all
$ p11-kit list-tokens "pkcs11:token=tpm2-token"
```

Display supported mechanisms:
```all
$ p11-kit list-mechanisms "pkcs11:token=tpm2-token"
```

Create an RSA key:
```all
$ p11-kit generate-keypair --label rsa2048 --type rsa --bits 2048 --login "pkcs11:token=tpm2-token;pin-value=userpin"
```

Create an ECC key:
```all
$ p11-kit generate-keypair --label eccp256 --type ecdsa --curve secp256r1 --login "pkcs11:token=tpm2-token;pin-value=userpin"
```

List the created keys:
```all
$ p11-kit list-objects --login "pkcs11:object=rsa2048;pin-value=userpin"
$ p11-kit list-objects --login "pkcs11:object=eccp256;pin-value=userpin"
```

Read the public component of the RSA key:
```all
$ p11-kit export-object --login "pkcs11:token=tpm2-token;object=rsa2048;type=public;pin-value=userpin" > rsa.pub.pem
```

Read the public component of the ECC key:
```all
$ p11-kit export-object --login "pkcs11:token=tpm2-token;object=eccp256;type=public;pin-value=userpin" > ecc.pub.pem
```

Import a dummy certificate:
```all
$ openssl req -x509 -newkey rsa:2048 -keyout ca.key.pem -out ca.cert.pem -sha256 -days 365 -nodes -subj "/CN=dummy"
$ p11-kit import-object --file ca.cert.pem --label dummy-cert --login "pkcs11:token=tpm2-token;pin-value=userpin"
$ p11-kit list-objects "pkcs11:object=dummy-cert"
```

Export the dummy certificate:
```all
$ p11-kit export-object --login "pkcs11:token=tpm2-token;object=dummy-cert;pin-value=userpin" > ca.key.pem
```

Destroy the keys and certificate:
```all
$ p11-kit delete-object --login "pkcs11:object=rsa2048;type=public;pin-value=userpin"
$ p11-kit delete-object --login "pkcs11:object=rsa2048;type=private;pin-value=userpin"
$ p11-kit delete-object --login "pkcs11:object=eccp256;type=public;pin-value=userpin"
$ p11-kit delete-object --login "pkcs11:object=eccp256;type=private;pin-value=userpin"
$ p11-kit delete-object --login "pkcs11:object=dummy-cert;pin-value=userpin"
```

<!--section-end-->

## p11tool from GnuTLS
<!--section:tools-p11tool-->

The p11tool from GnuTLS is a command-line tool designed to interact with PKCS#11 tokens, utilizing the libp11-kit library from the [p11-kit](https://github.com/p11-glue/p11-kit) project.

The p11tool is installed during the setup stage through the gnutls-bin package, but it is overwritten by installing the GnuTLS project from source.

To set the pin, you may use the environment variable `GNUTLS_PIN`/`GNUTLS_SO_PIN` or include `pin-value=` in the URL.

Set the pin:
```all
$ export GNUTLS_PIN="userpin"
$ export GNUTLS_SO_PIN="sopin"
```

Check the p11tool version:
```all
$ p11tool --version
```

List available tokens:
```all
$ p11tool --list-tokens
```

Change then restore the SO pin:
```all
$ GNUTLS_SO_PIN=sopin GNUTLS_NEW_SO_PIN=sopin2 p11tool --initialize-so-pin "pkcs11:token=tpm2-token"
$ GNUTLS_SO_PIN=sopin2 GNUTLS_NEW_SO_PIN=sopin p11tool --initialize-so-pin "pkcs11:token=tpm2-token"
```

Change the user pin using SO credentials (for demonstration purposes, change to the same pin).
```all
# This is required for the FAPI backend; otherwise, the overwrite check will fail.
$ rm -rf ${FAPI_METADATA_DIR}/user/keystore/P_RSA2048SHA256/HS/SRK/tpm2-pkcs11-token-usr-00000001

$ p11tool --initialize-pin --so-login --set-pin userpin "pkcs11:token=tpm2-token;pin-value=sopin"
```

Display supported mechanisms:
```all
$ p11tool --list-mechanisms "pkcs11:token=tpm2-token"
```

Create an RSA key and output its public component:
```all
$ p11tool --generate-privkey rsa --bits 2048 --label rsa2048 --outfile rsa.pub.pem --login "pkcs11:token=tpm2-token?pin-value=userpin"
```

Create an ECC key and output its public component:
```all
$ p11tool --generate-privkey ecdsa --curve secp256r1 --label eccp256 --outfile ecc.pub.pem --login "pkcs11:token=tpm2-token?pin-value=userpin"
```

List all available objects:
```all
$ p11tool --list-all --login "pkcs11:token=tpm2-token?pin-value=userpin"
```

Import a dummy certificate:
```all
$ openssl req -x509 -newkey rsa:2048 -keyout ca.key.pem -out ca.cert.pem -sha256 -days 365 -nodes -subj "/CN=dummy"
$ p11tool --write --load-certificate ca.cert.pem --label dummy-cert --login "pkcs11:token=tpm2-token?pin-value=userpin"
$ p11tool --list-all-cert "pkcs11:token=tpm2-token"
```

Export the dummy certificate:
```all
$ p11tool --export-chain "pkcs11:token=tpm2-token;object=dummy-cert" --outfile ca.cert.pem
```

Destroy the keys and certificate:
```all
$ p11tool --delete --batch --login "pkcs11:token=tpm2-token;object=rsa2048?pin-value=userpin"
$ p11tool --delete --batch --login "pkcs11:token=tpm2-token;object=eccp256?pin-value=userpin"
$ p11tool --delete --batch --login "pkcs11:token=tpm2-token;object=dummy-cert?pin-value=userpin"
```

<!--section-end-->

## NSS Tools
<!--section:tools-nss-tools-->

Originally known as [Netscape Security Services, Network Security Services (NSS)](https://firefox-source-docs.mozilla.org/security/nss/index.html) is a suite of libraries designed to support the cross-platform development of security-enabled client and server applications. The NSS-tools includes tools for developing, debugging, and managing applications that utilize these libraries.

NSS command-line tools are installed during the setup stage via the `libnss3-tools` package.

Check the version of the `libnss3-tools` package and its dependencies:
```all
$ apt show libnss3-tools
$ apt-cache depends libnss3-tools | grep Depends: | cut -d ":" -f2 | xargs dpkg -l
```

Set the location of the NSS database path:
```all
$ export NSSDB_DIR=$(mktemp -d)
```

Create an empty NSS database:
> This process creates three files: `cert9.db`, `key4.db`, and `pkcs11.txt`. If no external token is specified, the internal slot will be used. An internal slot is a virtual slot maintained within the NSS software (NSS Internal PKCS #11 Module), and its associated certificate and key database are stored in `cert9.db` and `key4.db`, respectively. The list of loaded PKCS #11 modules is stored in `pkcs11.txt`.
```all
$ certutil -N -d "sql:$NSSDB_DIR" --empty-password
$ ls -la $NSSDB_DIR
```

Generate a noise file for seeding purposes:
```all
$ SLOT_INDEX=$(pkcs11-tool-tpm2 --list-token-slots | grep "): tpm2-token" | awk '{print $2}')
$ pkcs11-tool-tpm2 --slot-index $SLOT_INDEX --generate-random 32 --output-file noise.bin

# or

$ dd if=/dev/random of=noise.bin bs=32 count=1
```

<b><ins>Operations on NSS Internal PKCS #11 Module:</ins></b>

Display the list of modules and their associated slots:
```all
$ modutil -list -dbdir "sql:$NSSDB_DIR" | grep "NSS Internal PKCS #11 Module"
```

Display the details of the module:
```all
$ modutil -list "NSS Internal PKCS #11 Module" -dbdir "sql:$NSSDB_DIR"
```

Create a self-signed CA certificate and its associated CA key:
> Make it non-interactive by adding `echo -ne "y\ny"` to respond to the following prompts:
> - Is this a CA certificate [y/N]?
> - Enter the path length constraint, press enter to skip [<0 for unlimited path]:
> - Is this a critical extension [y/N]?
```all
$ echo -ne "y\ny" | certutil -S -s "CN=Software CA" \
    -n "software-ca" -x -t "C,C,C" -2 \
    -7 software-ca@example.com \
    --keyUsage certSigning,crlSigning,critical \
    --nsCertType objectSigningCA,critical \
    -d "sql:$NSSDB_DIR" -z noise.bin
```

List the CA key:
```all
$ certutil -K -d "sql:$NSSDB_DIR" | grep "NSS Certificate DB:software-ca"
```

List the CA certificate:
```all
$ certutil -L -d "sql:$NSSDB_DIR" | grep "software-ca"
```

Print the information of the CA certificate:
```all
# Output in PEM encoding
$ certutil -L -d "sql:$NSSDB_DIR" -a -n "software-ca" > ca.crt.pem
$ openssl x509 -in ca.crt.pem -text -noout

# Output in human-readable format directly
$ certutil -L -d "sql:$NSSDB_DIR" -n "software-ca"
```

Create a client key and its associated Certificate Signing Request (CSR):
```all
$ certutil -R -k rsa -g 2048 -s "CN=Example,O=Example Corp,C=DE" \
    -7 software-client@example.com \
    -d "sql:$NSSDB_DIR" -z noise.bin -a -o client.csr.pem
```

Print the information of the client CSR:
```all
$ openssl req -in client.csr.pem -text -noout
```

List the client key:
> The key will remain in an "orphan" state until the associated certificate is added.
```all
$ certutil -K -d "sql:$NSSDB_DIR" | grep "orphan"
```

Convert the CSR from PEM encoding to DER encoding:
```all
$ openssl req -in client.csr.pem -outform DER -out client.csr.der
```

Use the software CA to issue a certificate from the CSR:
```all
# When the "-m" option is not specified in certutil, the serial number
# is generated based on the current time. Therefore, to ensure the
# uniqueness of the serial number, introduce a delay of 1 second.
$ sleep 1

$ certutil -C -c "software-ca" -i client.csr.der -o client.crt.der \
    -v 12 -w -1 -d "sql:$NSSDB_DIR" \
    --keyUsage digitalSignature,keyEncipherment,critical \
    -7 software-client@example.com
```

Add the client certificate to the NSS Certificate DB:
```all
$ certutil -A -n "software-client" -t ",," -d "sql:$NSSDB_DIR" -i client.crt.der
```

Print the information of the client certificate:
```all
# Output in PEM encoding
$ certutil -L -d "sql:$NSSDB_DIR" -a -n "software-client" > client.crt.pem
$ openssl x509 -in client.crt.pem -text -noout

# Output in human-readable format directly
$ certutil -L -d "sql:$NSSDB_DIR" -n "software-client"
```

List the client certificate chain:
```all
$ certutil -O -d "sql:$NSSDB_DIR" -n "software-client"
```

List the client certificate:
```all
$ certutil -L -d "sql:$NSSDB_DIR" | grep "software-client"
```

List the client key:
> The key will no longer be in an "orphan" state.
```all
$ certutil -K -d "sql:$NSSDB_DIR" | grep "NSS Certificate DB:software-client"
```

Create a message:
```all
$ echo "Hello world!" > message.plain
```

Perform digital signing and verification using the client key:
```all
# Sign
$ cmsutil -S -d "sql:$NSSDB_DIR" -N "software-client" -i message.plain -o message.sig.cms

# Verify
$ cmsutil -D -d "sql:$NSSDB_DIR" -i message.sig.cms
```

Perform digital signing and verification using the client key:
> Use the `-T` option to suppress content in the CMS message. This means the message will be detached from the signature file.
> During signature verification, the message must be explicitly provided through the `-c` detached content option.
```all
# Sign
$ cmsutil -S -d "sql:$NSSDB_DIR" -N "software-client" -T -i message.plain -o message.sig.cms

# Verify
$ cmsutil -D -d "sql:$NSSDB_DIR" -i message.sig.cms -c message.plain
```

Perform message encryption and decryption using the client key:
```all
# Encrypt
$ cmsutil -E -d "sql:$NSSDB_DIR" -r "software-client" -i message.plain -o message.cipher.cms

# Decrypt
$ cmsutil -D -d "sql:$NSSDB_DIR" -i message.cipher.cms -o message.decipher.cms
$ diff message.plain message.decipher.cms
```

Delete the client key and its associated certificate:
> Optionally, delete only the client certificate while retaining the key by: `certutil -D -d "sql:$NSSDB_DIR" -n "software-client"`
```all
$ certutil -F -d "sql:$NSSDB_DIR" -n "software-client"
```

<b><ins>Operations on TPM2-based PKCS #11 Module:</ins></b>

Add the TPM2-based PKCS #11 module to the NSS database:
```all
$ echo -ne "\n" | modutil -add "TPM2-based PKCS #11 Module" \
    -libfile "${HOME}/tpm2-pkcs11/src/.libs/libtpm2_pkcs11.so" \
    -dbdir "sql:$NSSDB_DIR"
```

Display the list of modules and their associated slots:
```all
$ modutil -list -dbdir "sql:$NSSDB_DIR" | grep "TPM2-based PKCS #11 Module"
$ modutil -list -dbdir "sql:$NSSDB_DIR" | grep "tpm2-token"
```

Display the details of the module:
```all
$ modutil -list "TPM2-based PKCS #11 Module" -dbdir "sql:$NSSDB_DIR"
```

Disable and then re-enable the module:
```all
$ echo -ne "\n" | modutil -disable "TPM2-based PKCS #11 Module" -slot "tpm2-token" -dbdir "sql:$NSSDB_DIR"
$ echo -ne "\n" | modutil -enable "TPM2-based PKCS #11 Module" -slot "tpm2-token" -dbdir "sql:$NSSDB_DIR"
```

Save the user PIN to a file for non-interactive setup:
> This is for demonstration purposes only. It is not secure to save the user PIN in a file that can be accessed by anyone.
```all
$ echo "userpin" > userpin.txt
```

Create a self-signed CA certificate and its associated CA key:
> Make it non-interactive by adding `echo -ne "y\ny"` to respond to the following prompts:
> - Is this a CA certificate [y/N]?
> - Enter the path length constraint, press enter to skip [<0 for unlimited path]:
> - Is this a critical extension [y/N]?

> Although the operation is successful, it will report an error in the background because the current version of tpm2-pkcs11 does not support the vendor-defined object class `CKO_NSS_TRUST`. This error occurs during the creation of object class `CKO_NSS_TRUST` through `C_CreateObject`. This error may not be fatal here but may have side effects on subsequent operations. Use at your own risk.
```all
$ echo -ne "y\ny" | certutil -S -h "tpm2-token" -f "userpin.txt" \
    -s "CN=TPM2 CA" -7 tpm2-ca@example.com \
    -n "tpm2-ca" -x -t "C,C,C" -2 \
    --keyUsage certSigning,crlSigning,critical \
    --nsCertType objectSigningCA,critical \
    -d "sql:$NSSDB_DIR" -z noise.bin
```

Create a TPM2 key and its associated Certificate Signing Request (CSR):
```all
$ certutil -R -k rsa -g 2048 -s "CN=Example2,O=Example2 Corp,C=DE" \
    -d "sql:$NSSDB_DIR" -h "tpm2-token" -f "userpin.txt" \
    -7 tpm2-client@example.com \
    -z noise.bin -a -o tpm2-client.csr.pem
```

Print the information of the client CSR:
```all
$ openssl req -in tpm2-client.csr.pem -text -noout
```

List the TPM2 key:
> The key will remain in an "orphan" state until the associated certificate is added.
```all
$ certutil -K -d "sql:$NSSDB_DIR" -h "tpm2-token" -f "userpin.txt" | grep "orphan"
```

Convert the CSR from PEM encoding to DER encoding:
```all
$ openssl req -in tpm2-client.csr.pem -outform DER -out tpm2-client.csr.der
```

Use the TPM2 CA to issue a certificate from the CSR:
```all
# When the "-m" option is not specified in certutil, the serial number
# is generated based on the current time. Therefore, to ensure the
# uniqueness of the serial number, introduce a delay of 1 second.
$ sleep 1

$ ~/pkcs11-optiga-tpm/scripts/cmdAutoFillPw.sh userpin \
    certutil -C -c "tpm2-token:tpm2-ca" \
    -i tpm2-client.csr.der -o tpm2-client.crt.der \
    -v 12 -w -1 -d "sql:$NSSDB_DIR" \
    --keyUsage digitalSignature,keyEncipherment,critical \
    -7 tpm2-client@example.com
```

Add the client certificate:
> The same non-fatal error related to `CKO_NSS_TRUST` will be seen here.

> Trust flag "u" is set automatically if the private key is present in the token.
```all
$ certutil -A -n "tpm2-client" -t ",," -d "sql:$NSSDB_DIR" \
    -i tpm2-client.crt.der -h "tpm2-token" -f "userpin.txt"
```

Print the information of the client certificate:
```all
# Output in PEM encoding
$ certutil -L -d "sql:$NSSDB_DIR" -a -n "tpm2-client" > tpm2-client.crt.pem
$ openssl x509 -in tpm2-client.crt.pem -text -noout

# Output in human-readable format directly
$ certutil -L -d "sql:$NSSDB_DIR" -n "tpm2-client"
```

List the client certificate chain:
```all
$ ~/pkcs11-optiga-tpm/scripts/cmdAutoFillPw.sh userpin certutil -O \
    -d "sql:$NSSDB_DIR" -h "pkcs11:token=tpm2-token" -n "tpm2-token:tpm2-client"
```

List the client certificate:
```all
$ certutil -L -d "sql:$NSSDB_DIR" -h "pkcs11:token=tpm2-token" -f "userpin.txt" \
    | grep "tpm2-token:tpm2-client"
```

List the TPM2 key:
> The key will no longer be in an "orphan" state.
```all
$ certutil -K -d "sql:$NSSDB_DIR" -h "pkcs11:token=tpm2-token" -f "userpin.txt" \
    | grep "tpm2-token:tpm2-client"
```

Create a message:
```all
$ echo "Hello world!" > message.txt
```

Perform digital signing and verification using the client key:
```all
# Sign
$ cmsutil -S -d "sql:$NSSDB_DIR" -N "tpm2-token:tpm2-client" \
    -f "userpin.txt" -i message.plain -o message.sig.cms

# Verify
$ cmsutil -D -d "sql:$NSSDB_DIR" -i message.sig.cms
```

Perform digital signing and verification using the client key:
> Use the `-T` option to suppress content in the CMS message. This means the message will be detached from the signature file.
> During signature verification, the message must be explicitly provided through the `-c` detached content option.
```all
# Sign
$ cmsutil -S -d "sql:$NSSDB_DIR" -N "tpm2-token:tpm2-client" \
    -f "userpin.txt" -T -i message.plain -o message.sig.cms

# Verify
$ cmsutil -D -d "sql:$NSSDB_DIR" -i message.sig.cms -c message.plain
```

Perform message encryption and decryption using the client key:
> Message decryption is not functioning because tpm2-pkcs11 does not support `C_UnwrapKey` and session objects, and its implementation of `C_CreateObject` does not support creating objects of the class `CKO_SECRET_KEY`, which are necessary to store the message decryption AES key during the decryption process.
```all
# Encrypt
$ ~/pkcs11-optiga-tpm/scripts/cmdAutoFillPw.sh userpin cmsutil -E -d "sql:$NSSDB_DIR" \
    -r "tpm2-token:tpm2-client" -i message.plain -o message.cipher.cms

# Decrypt
# Skipped
```

Delete the client key and its associated certificate:
> Optionally, delete only the client certificate while retaining the key by: `certutil -D -d "sql:$NSSDB_DIR" -h "tpm2-token" -f "userpin.txt" -n "tpm2-token:tpm2-client"`
```all
$ certutil -F -d "sql:$NSSDB_DIR" -h "tpm2-token" -f "userpin.txt" -n "tpm2-token:tpm2-client"
```

Housekeeping:
```all
$ rm -rf $NSSDB_DIR
$ unset NSSDB_DIR
```

<!--section-end-->

---

# Housekeeping

## Housekeeping (ESYSDB)
<!--section:housekeeping-esysdb-->

Clear the TPM:
```all
$ tpm2_clear -c p
```

Remove the ESYSDB database:
```all
$ rm -rf $TPM2_PKCS11_STORE
$ unset TPM2_PKCS11_STORE
```

This is only necessary if the platform hierarchy is utilized:
```all
$ tpm2_evictcontrol -C p -c 0x81800001 || true
```

<!--section-end-->

## Housekeeping (FAPI)
<!--section:housekeeping-fapi-->

Clear the TPM:
```all
$ tpm2_clear -c p
```

Remove the FAPI metadata:
```all
$ rm -rf $FAPI_METADATA_DIR
$ unset FAPI_METADATA_DIR
```

<!--section-end-->

---

# GitHub Actions

This section is intended for maintainers only.

GitHub Actions has been set up to automate the testing of this integration guide as part of the CI/CD process. The testing methodology involves extracting command lines from the markdown and running them in a Docker container. Please refer to [script.sh](.github/docker/script.sh) to see the preparation of commands for testing. The process occurs seamlessly in the background, utilizing hidden tags in the README.md (visible in raw mode).

Commands are divided into sections using the markers `<!--section:xxx-->` and `<!--section-end-->`. This structure allows for the execution of specific sections multiple times (e.g., housekeeping) and allows for the flexibility of reordering or creating new test sequences. The test sequence construction (e.g., section 1 -> section 2 -> section 3 -> ...) is configurable.

To learn about the CI test sequence, refer to the README.md in raw mode and specifically look for `<!--tests:`.

For debugging, it is possible to download the executed test scripts from the Artifacts of a GitHub Actions run.

---

# License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

<!--tests:
setup-pt1;setup-esysdb-install;setup-pt2;

backend-init-esysdb;token-init-pkcs11-tool;tools-pkcs11-tool;housekeeping-esysdb;
backend-init-esysdb;token-init-pkcs11-tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;token-init-tpm2-ptool-link-keys-platform;token-init-tpm2-ptool-import-keys-certs;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;tools-pkcs11-tool;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-pkcs11-tool;housekeeping-esysdb

backend-init-esysdb;token-init-pkcs11-tool;tools-p11-kit;housekeeping-esysdb;
backend-init-esysdb;token-init-pkcs11-tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;token-init-tpm2-ptool-link-keys-platform;token-init-tpm2-ptool-import-keys-certs;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;tools-p11-kit;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11-kit;housekeeping-esysdb

backend-init-esysdb;token-init-pkcs11-tool;tools-p11tool;housekeeping-esysdb;
backend-init-esysdb;token-init-pkcs11-tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;token-init-tpm2-ptool-link-keys-platform;token-init-tpm2-ptool-import-keys-certs;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;tools-p11tool;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-p11tool;housekeeping-esysdb

backend-init-esysdb;token-init-pkcs11-tool;tools-nss-tools;housekeeping-esysdb;
backend-init-esysdb;token-init-pkcs11-tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-default-primary-key;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-storage;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-tpm2-ptool-custom-primary-key-platform;token-init-tpm2-ptool-link-keys-platform;token-init-tpm2-ptool-import-keys-certs;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;tools-nss-tools;housekeeping-esysdb
backend-init-esysdb;token-init-p11tool;token-init-tpm2-ptool-link-keys-storage;token-init-tpm2-ptool-import-keys-certs;token-init-key-import;tools-nss-tools;housekeeping-esysdb

setup-fapi-install;

backend-init-fapi;token-init-pkcs11-tool;token-init-key-import;tools-pkcs11-tool;housekeeping-fapi;
backend-init-fapi;token-init-p11tool;token-init-key-import;tools-pkcs11-tool;housekeeping-fapi

backend-init-fapi;token-init-pkcs11-tool;token-init-key-import;tools-p11-kit;housekeeping-fapi;
backend-init-fapi;token-init-p11tool;token-init-key-import;tools-p11-kit;housekeeping-fapi

backend-init-fapi;token-init-pkcs11-tool;token-init-key-import;tools-p11tool;housekeeping-fapi;
backend-init-fapi;token-init-p11tool;token-init-key-import;tools-p11tool;housekeeping-fapi

backend-init-fapi;token-init-pkcs11-tool;token-init-key-import;tools-nss-tools;housekeeping-fapi;
backend-init-fapi;token-init-p11tool;token-init-key-import;tools-nss-tools;housekeeping-fapi
-->
