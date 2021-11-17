# Introduction

This section contains the guide to create a TPM-based PKCS #11 token using FAPI as token backend.

- Use FAPI instead of esysdb (SQLite) to provide disk storage service to [tpm2-pkcs11](https://github.com/tpm2-software/tpm2-pkcs11) ([read more](https://github.com/tpm2-software/tpm2-pkcs11/blob/master/docs/FAPI.md)).
- Provide an option to link a TPM persistent key to a PKCS #11 token 
- You will need a [Raspberry Pi 4](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/) and an [Infineon IRIDIUM9670 TPM2.0 board](https://www.infineon.com/cms/en/product/evaluation-boards/iridium9670-tpm2.0-linux/)
- You may use a TPM simulator, but it is not covered in this guide

# Table of Contents

**[Download Repositories](#download-repositories)**<br>
**[Install Dependencies](#install-dependencies)**<br>
**[Install tpm2-tss](#install-tpm2-tss)**<br>
**[Install tpm2-tools](#install-tpm2-tools)**<br>
**[Install tpm2-pkcs11](#install-tpm2-pkcs11)**<br>
**[Install OpenSC](#install-opensc)**<br>
**[Enable Hardware TPM](#enable-hardware-tpm)**<br>
**[Environment Setup](#environment-setup)**<br>
**[Provision TPM](#provision-tpm)**<br>
**[Create PKCS #11 Token](#create-pkcs-11-token)**<br>
**[Import TPM Persistent Key Pair (RSA)](#import-tpm-persistent-key-pair-rsa)**<br>

<hr>

# Download Repositories

```
$ git clone https://github.com/Infineon/pkcs11-optiga-tpm ~/pkcs11-optiga-tpm
$ git clone --branch 3.1.0 https://github.com/tpm2-software/tpm2-tss ~/tpm2-tss
$ git clone --branch 5.1.1 https://github.com/tpm2-software/tpm2-tools ~/tpm2-tools
$ git clone --branch 1.6.0 https://github.com/tpm2-software/tpm2-pkcs11 ~/tpm2-pkcs11
$ git clone --branch 0.21.0 https://github.com/OpenSC/OpenSC ~/OpenSC
```

# Install Dependencies

```
$ sudo apt update
$ sudo apt -y install autoconf-archive libcmocka0 libcmocka-dev \
  procps iproute2 build-essential git pkg-config gcc \
  libtool automake libssl-dev uthash-dev autoconf doxygen \
  libgcrypt-dev libjson-c-dev libcurl4-gnutls-dev uuid-dev \
  pandoc libglib2.0-dev libsqlite3-dev libyaml-dev
```

# Install tpm2-tss

```
$ cd ~/tpm2-tss
$ ./bootstrap
$ ./configure
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```

# Install tpm2-tools

```
$ cd ~/tpm2-tools
$ ./bootstrap
$ ./configure
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```

# Install tpm2-pkcs11

```
$ cd ~/tpm2-pkcs11
$ ./bootstrap
$ ./configure --enable-fapi
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```

# Install OpenSC

Install additional dependencies:
```
$ sudo apt install libpcsclite-dev
```

Install OpenSC:
```
$ cd ~/OpenSC
$ ./bootstrap
$ ./configure
$ make -j$(nproc)
$ sudo make install
$ sudo ldconfig
```

# Enable Hardware TPM

Load the TPM overlay:

```
$ sudo su -c "echo 'dtoverlay=tpm-slb9670' >> /boot/config.txt"
```

Power down your Raspberry Pi and attach the [Iridium 9670 TPM 2.0 board](https://www.infineon.com/cms/en/product/evaluation-boards/iridium9670-tpm2.0-linux/) according to the following image:

<p align="center">
    <img src="https://github.com/Infineon/pkcs11-optiga-tpm/raw/main/media/raspberry-with-tpm.jpg" width="50%">
</p>

Power up your Raspberry Pi and check if the TPM is enabled by looking for the device nodes:

```
$ ls /dev | grep tpm
/dev/tpm0
/dev/tpmrm0
```

Grant access permission:
```
$ sudo chmod a+rw /dev/tpm0
$ sudo chmod a+rw /dev/tpmrm0
```

# Environment Setup

Set tcti:
```
$ export TPM2_PKCS11_TCTI="device:/dev/tpmrm0"
```

Use FAPI instead of esysdb (SQLite) as backend:
```
$ export TPM2_PKCS11_BACKEND="fapi"
```

For convenience's sake, use alias to shorten the command:
```
$ alias tpm2pkcs11-tool='pkcs11-tool --module /usr/local/lib/libtpm2_pkcs11.so'
```

# Provision TPM

1. Recommended change in `/usr/local/etc/tpm2-tss/fapi-config.json`:
    - Move all directories to user space, hence prevent access permission issues and `double free or corruption` regression
    - Change profile to `P_ECCP256SHA256` instead of `P_RSA2048SHA256` to increase the performance:
    ```
    {
        "profile_name": "P_ECCP256SHA256",
        "profile_dir": "/usr/local/etc/tpm2-tss/fapi-profiles/",
        "user_dir": "/home/pi/.local/share/tpm2-tss/user/keystore",
        "system_dir": "/home/pi/.local/share/tpm2-tss/system/keystore",
        "tcti": "",
        "system_pcrs" : [],
        "log_dir" : "/home/pi/.local/share/tpm2-tss/eventlog/"
    }
    ```
2. Reset the FAPI database by emptying the user_dir and system_dir.
3. Clear TPM:
    ```
    $ tpm2_clear -c p
    ```
4. Provision TPM using FAPI.<br>
	Storage Root Key (SRK) will be created based on the profile `profile_dir/profile_name` and made persistent at handle `0x81000001` (specified in the profile too) without authorization value. TPM metadata is stored in the directory `system_dir`.
    ```
    $ tss2_provision
    ```

# Create PKCS #11 Token

1. Create a token. Token metadata is stored in the directory `user_dir`:
    ```
    $ tpm2pkcs11-tool --slot-index=0 --init-token --label="pkcs11 token" --so-pin="sopin"
    ```
2. Set user pin:
    ```
    $ tpm2pkcs11-tool --slot-index=0 --init-pin --so-pin="sopin" --login --pin="userpin"
    ```
3. Show the token:
    ```
    $ tpm2pkcs11-tool --list-token-slots
    ```

# Import TPM Persistent Key Pair (RSA)

1. Create an RSA key pair and make it persistent. 3 options:
	- Create it under the SRK `0x81000001`:
	  ```
      $ tpm2_create -G rsa2048 -g sha256 -C 0x81000001 -u rsakey.pub -r rsakey.priv -a "fixedtpm|fixedparent|sensitivedataorigin|userwithauth|decrypt|sign" -p authvalue
      $ tpm2_load -C 0x81000001 -u rsakey.pub -r rsakey.priv -c rsakey.ctx
      $ tpm2_evictcontrol -c rsakey.ctx 0x81000002
	  ```
	- Before creating an RSA key pair, create an identical SRK following the profile `P_ECCP256SHA256`. It seems redundant but useful when a TPM is going to be pre-provisioned before installing it on a host:
	  ```
	  $ tpm2_createprimary -C o -G ecc256 -g sha256 -c parent.ctx
      $ tpm2_create -G rsa2048 -g sha256 -C parent.ctx -u rsakey.pub -r rsakey.priv -a "fixedtpm|fixedparent|sensitivedataorigin|userwithauth|decrypt|sign" -p authvalue
      $ tpm2_load -C parent.ctx -u rsakey.pub -r rsakey.priv -c rsakey.ctx
      $ tpm2_evictcontrol -c rsakey.ctx 0x81000002
	  ```
	- Create it under the platform hierarchy. The permitted handle range is different from storage hierarchy:
	  ```
	  $ tpm2_createprimary -C p -G ecc256 -g sha256 -c parent.ctx
	  $ tpm2_evictcontrol -C p -c parent.ctx 0x81800001
	  $ tpm2_create -G rsa2048 -g sha256 -C 0x81800001 -u rsakey.pub -r rsakey.priv -a "fixedtpm|fixedparent|sensitivedataorigin|userwithauth|decrypt|sign" -p authvalue
	  $ tpm2_load -C 0x81800001 -u rsakey.pub -r rsakey.priv -c rsakey.ctx
	  $ tpm2_evictcontrol -C p -c rsakey.ctx 0x81800002
	  ```
      Platform objects cannot be cleared conveniently using the command `tpm2_clear -c p`. These keys can still be evicted by:
      ```
      $ tpm2_evictcontrol -C p -c 0x81800001
      $ tpm2_evictcontrol -C p -c 0x81800002
      ```

2. Import of persistent keys to PKCS #11 token is not supported in [1.6.0](https://github.com/tpm2-software/tpm2-pkcs11/tree/1.6.0), hence the need to apply a patch:
	```
	$ cd ~/tpm2-pkcs11
	$ git am ~/pkcs11-optiga-tpm/fapi-backend/patches/implement-custom-api-c_importkeypair.patch
	```
3. Reinstall the tpm2-pkcs11 following section [Install tpm2-pkcs11](#install-tpm2-pkcs11).
4. Using a tool to invoke the key import operation:
    - Import key handle `0x81000002`:
      ```
      $ cd ~/pkcs11-optiga-tpm/import-tool
      $ gcc -Wall -g -DPERSISTENT_HANDLE=0x81000002 -DAUTH_VALUE=\"authvalue\" -o import import.c -ltpm2_pkcs11
      $ ./import
      ```
    - Import key handle `0x81800002`
      ```
      $ cd ~/pkcs11-optiga-tpm/import-tool
      $ gcc -Wall -g -DPERSISTENT_HANDLE=0x81800002 -DAUTH_VALUE=\"authvalue\" -o import import.c -ltpm2_pkcs11
      $ ./import
      ```
5. Test the imported key:
	```
	# List key objects
	$ tpm2pkcs11-tool --slot 1 --list-objects --login --pin userpin
	
	# Output the public component of an imported key
	$ tpm2pkcs11-tool --slot 1 --login --pin userpin --id 8081 --type pubkey --read-object > rsa.pub.der
	$ openssl rsa -inform DER -outform PEM -in rsa.pub.der -pubin > rsa.pub.pem
	
	# Generate test data
	$ tpm2pkcs11-tool --slot 1 --generate-random 32 > data
	
	# Perform encryption and decryption
	$ openssl rsautl -encrypt -inkey rsa.pub.pem -in data -pubin -out data.crypt
	$ tpm2pkcs11-tool --slot 1 --login --pin userpin --id 8081 --decrypt --mechanism RSA-PKCS --input-file data.crypt --output-file data.plain
	$ diff data data.plain
	
	# Perform signing and verification
	$ tpm2pkcs11-tool --slot 1 --id 8081 --login --pin userpin --sign --mechanism SHA256-RSA-PKCS --input-file data --output-file data.rsa.sig
	$ openssl dgst -sha256 -verify rsa.pub.pem -signature data.rsa.sig data
	
	# Delete the key
	$ tpm2pkcs11-tool --slot 1 --login --pin userpin --delete-object --type privkey --id 8081
	$ tpm2pkcs11-tool --slot 1 --login --pin userpin --delete-object --type pubkey --id 8081
	```
