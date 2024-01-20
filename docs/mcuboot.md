# Project: mcuboot
- Documenting the steps along the way of studying and developing a working mcuboot application

## References
- [Sample with MCUboot](https://docs.zephyrproject.org/latest/samples/application_development/sysbuild/with_mcuboot/README.html)
- [Sysbuild (System build)](https://docs.zephyrproject.org/latest/build/sysbuild/index.html)

## Zephyr sysbuild
- Higher level build system used to combine multiple other build systems together in a hierarchical build system
- Example:
    - It is possible to use `sysbuild` to build a Zephyr application + the mcuboot bootloader as well as flash both of them onto your device, and debug...
- `Sysbuild` works by configuring and building at least one Zephyr application and, optionally, as many additional projects as required
- Building with `sysbuild` can be done via `west`
    - `west build -b reel_board --sysbuild samples/hello_world`
- Or via `CMake`
    - `cmake -Bbuild -GNinja -DBOARD=reel_board -DAPP_DIR=samples/hello_world share/sysbuild`
    - `ninja -Cbuild`
- It is possible to configure west build to use `sysbuild` by default if desired
- There is a concept of Configuration namespacing:
    - When building a single Zephyr app, the CMake settings and/or KConfig build options are handled by Zephyr build system
    - When using `sysbuild` and combining multiple Zephyr build systems, there could be settings exclusive to sysbuild and not used by any of the applications
    - namespaces can be used to specify which variables are to be 'sent' to one of the multiple possible Zephyr build systems and which to maintain only at a sysbuild level 
    - Example:
        - `west build -b reel_board --sysbuild samples/hello_world -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_DEBUG_OPTIMIZATIONS=y -Dmcuboot_DEBUG_OPTIMIZATIONS=y`
    - This will add mcuboot as the bootloader at a `sysbuild` level (see "-DSB_xxx") whereas it'll add debug optimizations to both images - application & boot (see there's only "-Dxxx")
- There is also the concept of CMake variable namespacing
    - It's similar to the previous one, but in this case it allows for the creation of namespaces and variables can be defined within that specific namespace
    - Example if you have a `sysbuild` env that is composed of 2 different independent projects and you want to define a compilation symbol on one of those projects, using the concept of namespaces it is possible to do so by following the pattern `-D<namespace>_<var>=<value>`
    - Example:
        - `west build --sysbuild ... -- -Dmy_sample_FOO=BAR`
- KConfig namespacing
    - Similar concept to the ones above, it's possible to specify a KConfig which is related at the `sysbuild` level or at one of the project's level with the pattern `-D<namespace>_CONFIG_<var>=<value>`
    - Example:
        - `west build --sysbuild ... -- -Dmy_sample_CONFIG_FOO=BAR`

## Sysbuild configuration

### sysbuild.conf
- It is possible to create a sysbuild.conf file to define Kconfig-like symbols that are relevant at a sysbuild level

### sysbuild folder
- It is possible to create a folder named `sysbuild` in the root of the project
- This folder might contain configurations that can be used during a build, targeting one specific image being built
- There are 2 approaches:
    - **Approach 1:**
    ```bash
    # Template
    zbox/sysbuild/<image_name>.overlay
    zbox/sysbuild/<image_name>.conf

    # Example:
    zbox/sysbuild/mcuboot.overlay
    zbox/sysbuild/mcuboot.conf
    ```
    - This approach will append configurations defined in `zbox/sysbuild/**` with the original configurations defined in the image's root directory
    - In the example of mcuboot given above, it will append the conf file defined in `zbox/sysbuild/mcuboot.conf` with the one present on `zephyrproject/bootloader/mcuboot/boot/zephyr/prj.conf`
    - This allows for a less invasive approach

    - **Approach 2:**
    ```bash
    # Template
    zbox/sysbuild/<image_name>/boards/<board>.overlay
    zbox/sysbuild/<image_name>/boards/<board>.conf
    zbox/sysbuild/<image_name>/prj.conf

    # Example:
    zbox/sysbuild/
                 └── mcuboot
                     ├── boards
                     │   ├── esp32c3_devkitm.conf
                     │   ├── esp32c3_devkitm.overlay
                     │   └── stm32f4_disco.overlay
                     └── prj.conf
    ```
    - This approach will override the configurations defined in that image's original root directory
    - In the example of mcuboot given above, it will stop using the configurations defined in `zephyrproject/bootloader/mcuboot/boot/zephyr/prj.conf` and `zephyrproject/bootloader/mcuboot/boot/zephyr/boards/**` and will only use the configurations defined within `zbox/sysbuild/mcuboot/**`
    - This allows user to take full control of the configuration of one of sysbuild's projects being built

- Unfortunately, because I want to support more then 1 board at the same time in the same project, the only approach viable seems to be approach 2.
    - Using approach 1 in my case works for `stm32f4_disco` but fails for `esp32c3_devkitm`
    - If there was a way to use approach 1 + specify it to take effect only for specific boards, that would be ideal. Something like `zbox/sysbuild/mcuboot_stm32f4_disco.overlay`
    - Because this is not possible, using approach 2 is the way to go. The bad part is that I had to copy most part of the contents of the original mcuboot conf and overlay files

## mcuboot
- mcuboot is a secure bootloader for 32-bit microcontrollers
- It is in mcuboot's scope
    - To define on a known image format and memory layout
    - Take decisions of whether to perform a swap, or a rollback, an image integrity check, etc.
- It is NOT in mcuboot's scope:
    - Define how the images will be managed
    - Define how the images will end up in the device's memory so that they can be swaped to


- Requirements to build mcuboot within a Zephyr application:
- Make sure there are flash partitions defined in the board's device tree
```
boot_partition: for MCUboot itself
slot0_partition: the primary slot of Image 0
slot1_partition: the secondary slot of Image 0
scratch_partition: the scratch slot (if needed)
```
- Set `CONFIG_BOOTLOADER_MCUBOOT` KConfig in prj.conf
- The 2 image slots must be contiguous
- If mcuboot is used as stage1 bootloader, boot partition should be configured so that it is ran first after a reset
- [The mcuboot design page](https://docs.mcuboot.com/design.html) will describe in deatail:
    - Bootloader design, high level operation and limitations
    - Image format
    - Flash layout and partitions dimensioning based on: memory sector sizes, flash wear and amount of desirable updates
    - Swap strategies and rollback capabilities
    - Boot swap types and how the bootloader decides which action to take upon reboot (image trailers)
    - Security capabilities

- I'll be using a [Swap using scratch](https://docs.mcuboot.com/design.html#image-swap-using-scratch) strategy

## mcumgr
- mcumgr is a management library for 32-bit MCUs.
- The goal of mcumgr is to define a common management infrastructure with pluggable transport and encoding components.
- The management is based on the Simple Management Protocol (SMP)
- mcumgr provides definitions and handlers for some core commands like image management
- There is a [mcumgr CLI tool written in Go](https://github.com/apache/mynewt-mcumgr-cli) that allows to update an image over BLE to devices running an mcumgr server.
- Zephyr tutorials are all based in the CLI tool written in Go, but at the same time the documentation itself states that this tool is for evaluation only and has bugs so it should not be used in production environments.
- Alternatively there is a [mcumgr client written in Rust](https://github.com/vouch-opensource/mcumgr-client/) as well that allows to update an image over a serial port to devices running an mcumgr server. (preferred over CLI)
- Both tools were installed in my experience in order to play with both of them:
```bash
# Install mcumgr CLI tool written in Go
ggm@gAN515-52:~ $ sudo apt install golang-go
ggm@gAN515-52:~ $ echo "PATH=$PATH:$HOME/go/bin" >> ~/.bashrc
ggm@gAN515-52:~ $ go install github.com/apache/mynewt-mcumgr-cli/mcumgr@latest
ggm@gAN515-52:~ $ which mcumgr
/home/ggm/go/bin/mcumgr

# Install mcumgr-client Rust tool (alternatively could install from source, compiling rust code)
ggm@gAN515-52:~ $ wget https://github.com/vouch-opensource/mcumgr-client/releases/download/v0.0.3/mcumgr-client-linux-x86.zip -P ~/Downloads/
ggm@gAN515-52:~ $ unzip ~/Downloads/mcumgr-client-linux-x86.zip -d ~/Downloads/
ggm@gAN515-52:~ $ sudo cp Downloads/mcumgr-client-linux-x86/mcumgr-client /usr/bin/
ggm@gAN515-52:~ $ which mcumgr-client 
/usr/bin/mcumgr-client
```

## Practical proof of concept
- Now it's time to put it all together in a minimal working example
- References:
    - [Zephyr mcumgr documentation](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html)
    - [Zephyr mcumgr example](https://docs.zephyrproject.org/latest/samples/subsys/mgmt/mcumgr/smp_svr/README.html)
    - [mcuboot information about image signing](https://docs.mcuboot.com/readme-zephyr.html)
- Created a project which is a combination between the `blinky` sample and the `subsys/mgmt/mcumgr/smp_svr` sample
- This example will use the [image management](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html#image-management) and the [statistics management](https://docs.zephyrproject.org/latest/services/device_mgmt/mcumgr.html#image-management) capabilities of the mcumgr.
- The plan was to produce 2 binaries, one which blinks the green LED at 1Hz (green bin) and another one that would blink the blue LED at 1Hz (blue bin) and swap between them using SMP protocol
- Pulled in the main KConfig configurations from the SMP Server sample in order to make sure that I have in fact a SMP server running in my embedded device
- The final sample code can be seen in `main.cpp`
- I then proceeded to build and flash the (mcuboot + green bin) in the target
```bash
./bbuild.sh -f -r -l --board stm32f4_disco
```
- After this is done, if you open a serial port at the console UART and reset the target, it's possible to see mcuboot logs, indicating the success of the project configuration
- Green LED should be blinking
- In order to check that the CLI/Client SMP tools are working properly as well as that my device in fact has an SMP server running, I performed some of the following basic commands
```bash
# Create a "conn device configuration" so that I do not have to specify it all the time
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr conn add ggmconn type="serial" connstring="dev=/dev/ttyUSB0,baud=115200,mtu=256"
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn echo hello
hello

# Testing the Statistics management capabilities
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn stat list
stat groups:
    smp_server_stats
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn stat smp_server_stats
stat group: smp_server_stats
      1928 counter_stat
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn stat smp_server_stats
stat group: smp_server_stats
      1931 counter_stat
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn stat smp_server_stats
stat group: smp_server_stats
      1931 counter_stat

# Testing the Image management capabilities
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image list
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active confirmed
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr-client -d /dev/ttyUSB0 list
mcumgr-client 0.0.3, Copyright © 2023 Vouch.io LLC

19:37:29 [INFO] send image list request
19:37:29 [INFO] response: {
  "images": [
    {
      "hash": [
        104,
        157,
        229,
        164,
        169,
        215,
        203,
        50,
        251,
        127,
        118,
        187,
        60,
        191,
        39,
        90,
        59,
        206,
        28,
        237,
        47,
        118,
        80,
        243,
        91,
        52,
        128,
        230,
        85,
        26,
        84,
        154
      ],
      "slot": 0,
      "active": true,
      "pending": false,
      "version": "0.0.0",
      "bootable": true,
      "confirmed": true,
      "permanent": false
    }
  ],
  "splitStatus": 0
}
```
- Then the next step is to rebuild the software, but now changing the compilation symbols in order to build the blue bin.
- If it is not already signed, sign the binary and transfer it to the target via SMP protocol
```bash
# Already signed:
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ find build -iname "*sign*"
build/zbox/zephyr/zephyr.signed.bin
build/zbox/zephyr/zephyr.signed.hex

# Manually signing it would be something like this:
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ west sign -t imgtool -- --key ../bootloader/mcuboot/root-rsa-2048.pem

# Option 1: Transfer the binary to the target with mcumgr CLI tool
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image upload -e build/zbox/zephyr/zephyr.signed.bin
 48.33 KiB / 48.33 KiB [========================================================================================================================] 100.00% 1.93 KiB/s 25s
Done


# Option 2: Transfer the binary to the target with mcumgr-client
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr-client -s 1 -m 512 -l 128 -d /dev/ttyUSB0 upload build/zbox/zephyr/zephyr.signed.bin 
mcumgr-client 0.0.3, Copyright © 2023 Vouch.io LLC

19:49:41 [INFO] upload file: build/zbox/zephyr/zephyr.signed.bin
19:49:41 [INFO] flashing to slot 1
19:49:41 [INFO] 49488 bytes to transfer
  [00:00:07] [================================================================================================================================] 48.33 KiB/48.33 KiB (0s)19:49:49 [INFO] upload took 8s
```

- Sometimes the process might fail. In that case start it over:
```bash
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr-client -s 1 -m 512 -l 128 -d /dev/ttyUSB0 upload build/zbox/zephyr/zephyr.signed.bin 
mcumgr-client 0.0.3, Copyright © 2023 Vouch.io LLC

19:49:32 [INFO] upload file: build/zbox/zephyr/zephyr.signed.bin
19:49:32 [INFO] flashing to slot 1
19:49:32 [INFO] 49488 bytes to transfer
19:49:37 [ERROR] Error: read error, expected: 6, read: 91
```
- Check that the second image is now present on the other slot of target's flash
```bash
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image list
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active confirmed
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
 image=0 slot=1
    version: 0.0.0
    bootable: true
    flags: 
    hash: 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
Split status: N/A (0)
```
- Indicate to mcuboot to perform the image SWAP on the next boot and restart the board (notice the change on the "flags" field of the image on slot=1)
```bash
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image test 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active confirmed
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
 image=0 slot=1
    version: 0.0.0
    bootable: true
    flags: pending
    hash: 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
Split status: N/A (0)
```

- After the reboot wait a little bit for mcuboot to perform the image swap and then the Blue LED should be blinking
- The image status should be like this
- It's possible to see that the images were swapped!
```bash
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image list
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active
    hash: 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
 image=0 slot=1
    version: 0.0.0
    bootable: true
    flags: confirmed
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
Split status: N/A (0)
```

- At this point, if you restart the target again, it'll go back to Green bin.
- In order for it to swap to Blue bin and stay there, you should "confirm" that that binary os "OK", indicating to the bootloader that is should NOT perform a rollack in the next reboot
- To swap and confirm the current image: (notice the change in the flags field)
```bash
# Swap
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image test 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active confirmed
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
 image=0 slot=1
    version: 0.0.0
    bootable: true
    flags: pending
    hash: 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
Split status: N/A (0)
# Confirm the current
(.venv) ggm@gAN515-52:~/zephyrproject/zbox (master)$ mcumgr -c ggmconn image confirm ""
Images:
 image=0 slot=0
    version: 0.0.0
    bootable: true
    flags: active confirmed
    hash: 12256572ab7223930fdbe7f27167307a4131ea9eba66756f9c8782ae7c5f6449
 image=0 slot=1
    version: 0.0.0
    bootable: true
    flags: 
    hash: 689de5a4a9d7cb32fb7f76bb3cbf275a3bce1ced2f7650f35b3480e6551a549a
Split status: N/A (0)

```
