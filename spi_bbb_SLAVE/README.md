# BeagleBone Black in SPI Slave Mode Project

## Issues and fixes

### Device tree overlay loaded check
First,  to load the device tree overlay, compile the .dts file with:

```bash
dtc -O dtb -o BB-SPI0-SLAVE-00A0.dtbo -b 0 -@ BB-SPI0-SLAVE-00A0.dts
```

Then, modify the `/boot/uEnv.txt` file:

1. Under `###Additional custom capes`, add:
```bash
uboot_overlay_addr4=/lib/firmware/BB-SPI0-SLAVE-00A0.dtbo
```

2. Under `###Custom Cape`, add:
```bash
dtb_overlay=/lib/firmware/BB-SPI0-SLAVE-00A0.dtbo
```

3. under `###Cape Universal Enable`, **COMMENT OUT THE LINE**:
```bash
enable_uboot_cape_universal=1 # <-- COMMENT THIS OUT!!!
```
This fixes the issue of
```
pinctrl-single 44e10800.pinmux: could not request pin 84 (PIN84) from group pinmux_bb_spi0_slave_pins  on device pinctrl-single 
```

To check if the overloay loaded correctly:
```bash
sudo cat /sys/kernel/debug/pinctrl/44e10800.pinmux-pinctrl-single/pinmux-pins 
```

and check pins 84-87, you should see something like "slave"

Also, check kernel logs and pipe it to `grep "spi|pinctrl|pinmux|slave"`. 


