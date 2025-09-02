
> This demonstrates a simple **SPI loopback communication** on the **BeagleBone Black**, where the same device acts as both **SPI master and slave**. The transmitted data (`tx`) is sent out and received back (`rx`) through a physical loopback wire.

---

## Features

- SPI communication using Linux’s `/dev/spidev1.0` interface
    
- Configures SPI mode, clock speed, and bits per word
    
- Performs full-duplex transfer (loopback) using `ioctl()`
    
- Prints transmitted and received data for verification
    

---

## Prerequisites

- BeagleBone Black (Tested running `Debian 6.3.0-18+deb9u1`)
    
- SPI interface enabled (spidev1.0)
    
- Loopback wire connecting pins as described

- Pin configuration using `config-pin`
    
- GCC compiler (tested using `gcc (Debian 6.3.0-18) 6.3.0 20170516`)
    
- SPI dev headers (`linux/spi/spidev.h`)
    

---

## Loopback Wiring (Physical Setup)

To perform the loopback, connect the SPI pins as follows:
> Don't forget `sudo`!

|BBB Pin|Connect To|Description|
|---|---|---|
|P9_18|P9_21|`spi0_d1` to `spi0_d0`|

[Reference BeagleBone Connectors Documentation](https://docs.beagleboard.org/boards/beaglebone/black/ch07.html#connector-p9)

---

## Pin configuration

We need to set pin modes of the P9 connector to spi. Use the following table and commands:

|PinMUX|Mode|Command|
|---|---|---|
|P9_17|`spi`|`sudo config-pin P9_17 spi`|
|P9_18|`spi`|`sudo config-pin P9_18 spi`|
|P9_21|`spi`|`sudo config-pin P9_21 spi`|
|P9_22|`spi`|`sudo config-pin P9_22 spi`|

## Code Breakdown

### 1. **Includes and Definitions**

```c
#include <linux/spi/spidev.h>
```

Imports SPI structures and IOCTL macros.

```c
#define MODE SPI_MODE_0
#define BITS 8
#define CLK_SPD 500000
#define DEVICE "/dev/spidev1.0"
```

- **MODE**: SPI mode 0 (CPOL = 0, CPHA = 0)
    
- **BITS**: 8-bit communication
    
- **CLK_SPD**: 500 kHz clock speed
    
- **DEVICE**: SPI bus device file on BBB
    

---

### 2. **Buffers and SPI Transfer Object**

```c
uint8_t tx[32] = {69};
uint8_t rx[32] = {0};
struct spi_ioc_transfer trx_obj;
```

- `tx`: Transmit buffer initialized with a test value (69).
    
- `rx`: Receive buffer initialized to zeros.
    
- `trx_obj`: Describes the transfer to be sent using `ioctl`.
    

```c
trx_obj.tx_buf = (unsigned long)tx;
trx_obj.rx_buf = (unsigned long)rx;
trx_obj.len = 32;
```

> Only the first byte (`tx[0]`) is meaningful; the rest are defaulted to 0.

---

### 3. **Open SPI Device**

```c
fd = open(DEVICE, O_RDWR);
```

Opens the SPI device file with read/write access.

---

### 4. **Configure SPI Parameters**

- **Mode Configuration:**
    

```c
ioctl(fd, SPI_IOC_RD_MODE32, &messenger);
messenger |= MODE;
ioctl(fd, SPI_IOC_WR_MODE32, &messenger);
```

- **Clock Speed:**
    

```c
ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &messenger);
messenger = CLK_SPD;
ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &messenger);
```

> These ensure the SPI controller operates at the correct mode and speed.

---

### 5. **Perform SPI Transfer**

```c
ioctl(fd, SPI_IOC_MESSAGE(1), &trx_obj);
```

- Initiates a full-duplex SPI transaction.
    
- Data from `tx` is sent, and the result is stored in `rx`.
    

---

### 6. **Output Results**

```c
printf("Initial value of rx buffer: %d\n", rx[0]);
printf("Final contents of rx buffer: %d\n", rx[0]);
```

- If loopback wiring is correct, `rx[0]` should match `tx[0]` (69).
    

---

## Build Instructions

**REMEMBER** to first use `config-pin` as described before, AND connect the correct pins together.

```bash
gcc -o bbb_spi bbb_spi.c
```

---

## Run the Program

```bash
sudo ./bbb_spi
```

> You need `sudo` to access `/dev/spidev1.0`.

---

## Sample Output

```
SPI Loopback with data: 69
Initial value of rx buffer: 0
------
Final contents of rx buffer: 69
```

> This confirms the loopback worked: transmitted data was received back.

---