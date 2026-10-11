# Galaxy100 (WB32 revision)

Epomaker Galaxy100, WB32FQ95 MCU, USB `342D:E489`, wired / 2.4G / Bluetooth.

* Keyboard Maintainer: [Epomaker](https://github.com/Epomaker) (original board files), community port
* Hardware Supported: Galaxy100 with WB32FQ95 (not the ES32 revision, USB `36B0:xxxx`)
* Hardware Availability: Galaxy100

Make example for this keyboard (after setting up your build environment):

    make epomaker/galaxy100:via

Flashing example for this keyboard:

    make epomaker/galaxy100:via:flash

To enter the bootloader, do one of the following (back switch on USB):

* **Fn + Right Shift + Esc**: keeps keymap, macros and settings
* Hold **Esc** while connecting the USB cable: also resets all settings to defaults
* Hold the reset switch on the bottom of the PCB while connecting the USB cable

See the project README for features and the VIA definition, `CHANGELOG.md` for versions and
`docs/hardware.md` for pins and verified hardware facts.
