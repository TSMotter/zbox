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