
>This is a C implementation to interface the **GY-271 QMC5883L** magnetometer sensor with the **BeagleBone Black** using the I2C protocol. It reads the magnetic field values along the **X**, **Y**, and **Z** axes and prints them continuously.

---

## 📌 Features

- Initializes the QMC5883L sensor using I2C
    
- Continuously reads 3-axis magnetic field data
    
- Outputs values in terminal at 2 Hz (every 500ms)
    

---

## 🧰 Prerequisites

- BeagleBone Black (with I2C enabled)
    
- GY-271 QMC5883L sensor
    
- Wiring between BBB and sensor
    
- GCC installed
    
- I2C tools and dev headers (for example: `libi2c-dev`)
    

---

## 🧠 Sensor I2C Details

|Property|Value|
|---|---|
|Default Address|`0x0D`|
|Interface|I2C|
|Register Width|8-bit|
|Data Width|16-bit (2 x 8-bit)|

---

## ⚡ Wiring Guide

|QMC5883L Pin|BBB Pin Name|BBB Header Pin|
|---|---|---|
|VCC|3.3V/5V|P9_3 or P9_5|
|GND|Ground|P9_1 or P9_2|
|SDA|I2C2_SDA|P9_20|
|SCL|I2C2_SCL|P9_19|

Enable I2C2 via device tree overlays or config.

---

## 🧾 Code Breakdown

### 1. Header Files

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
```

- Provides access to low-level I/O functions.
    
- `i2c-dev.h` provides constants and structures to interact with I2C devices in Linux.
    

---

### 2. Register Definitions

```c
#define QMC5883L_ADDRESS 0x0D
#define CONTROL_REG1 0x09
#define CONTROL_REG2 0x0A
#define SET_RESET_PERIOD_REGISTER 0x0B
```

- Sensor address and register mappings for configuring and reading data.
    

```c
#define X_LSB 0x00
#define X_MSB 0x01
#define Y_LSB 0x02
#define Y_MSB 0x03
#define Z_LSB 0x04
#define Z_MSB 0x05
```

- Data output registers (low byte and high byte for each axis).
    

---

### 3. Write Helper Function

```c
void writeRegister(int file, unsigned char reg, unsigned char value)
```

- Sends a 2-byte command (register address + value) to the sensor using `write()`.
    

---

### 4. Reading Magnetic Field Data

```c
void readMagneticField(int file, __int16_t *x, __int16_t *y, __int16_t *z)
```

This function performs the following:

1. Writes the register address to the sensor.
    
2. Reads the corresponding byte (LSB then MSB).
    
3. Combines the two bytes to form a signed 16-bit value.
    
4. Converts the value to positive if needed using 2's complement (optional, though usually not needed as `__int16_t` already handles sign).
    

```c
*x = (__int16_t)((x_msb[0] << 8) | x_lsb[0]);
```

> **Note**: The current code does 2's complement conversion manually, but this might not be necessary — it’s redundant for properly cast signed 16-bit values.

---

### 5. Main Function

```c
int main()
```

- Opens the I2C device file `/dev/i2c-2`.
    
- Sets the sensor's I2C address using `ioctl()`.
    
- Initializes the sensor with:
    
    - `SET_RESET_PERIOD_REGISTER = 0x01`
        
    - `CONTROL_REG1 = 0x1D` → Continuous mode, ODR = 200Hz, RNG = ±8G, OSR = 512
        
    - `CONTROL_REG2 = 0x00` → Default settings
        

```c
while (1)
{
    readMagneticField(file, &x, &y, &z);
    printf("Magnetic Field (uT) -> X: %d, Y: %d, Z: %d\n", x, y, z);
    usleep(500000); // 500 ms delay
}
```

- Reads the magnetic field data every 500 milliseconds and prints it to the terminal.

---

## 🛠 Build Instructions

```bash
gcc -o magfield magfield.c
```

---

## 🚀 Run

```bash
sudo ./magfield
```

> You may need root permissions to access `/dev/i2c-*`.

---

## 📤 Sample Output

```
Magnetic Field (uT) -> X: 87, Y: -34, Z: 125
Magnetic Field (uT) -> X: 85, Y: -35, Z: 128
Magnetic Field (uT) -> X: 90, Y: -30, Z: 130
```

---




