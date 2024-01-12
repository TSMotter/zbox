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
    - Approach 1:
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

    - Approach 2:
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

## mcuboot with Zephyr
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