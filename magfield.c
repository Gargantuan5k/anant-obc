#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#define QMC5883L_ADDRESS 0x0D // Default I2C address
#define CONTROL_REG1 0x09 // operational modes (MODE), output data update rate (ODR), magnetic field measurement range or sensitivity of the sensors (RNG) and over sampling rate (OSR)
#define CONTROL_REG2 0x0A // Interrupt Pin enabling (INT_ENB), Point roll over function enabling(POL_PNT) and soft reset (SOFT_RST)

// Data Registers
#define X_LSB 0x00
#define X_MSB 0x01
#define Y_LSB 0x02
#define Y_MSB 0x03
#define Z_LSB 0x04
#define Z_MSB 0x05

#define SET_RESET_PERIOD_REGISTER 0x0b // recommended as per datasheet

void writeRegister(int file, unsigned char reg, unsigned char value)
{
    unsigned char buffer[2] = {reg, value}; // reg to write to, val to be written
    write(file, buffer, 2);
}


// Get magnetic field (X, Y, Z) values from sensor
void readMagneticField(int file, __int16_t *x, __int16_t *y, __int16_t *z)
{
    unsigned char x_lsb[1];
    x_lsb[0] = X_LSB;

    unsigned char x_msb[1];
    x_msb[0] = X_MSB;

    unsigned char y_lsb[1];
    y_lsb[0] = Y_LSB;

    unsigned char y_msb[1];
    y_msb[0] = Y_MSB;

    unsigned char z_lsb[1];
    z_lsb[0] = Z_LSB;

    unsigned char z_msb[1];
    z_msb[0] = Z_MSB;
    
    write(file, x_lsb, 1);
    read(file, x_lsb, 1);

    write(file, x_msb, 1);
    read(file, x_msb, 1);

    write(file, y_lsb, 1);
    read(file, y_lsb, 1);

    write(file, y_msb, 1);
    read(file, y_msb, 1);

    write(file, z_lsb, 1);
    read(file, z_lsb, 1);

    write(file, z_msb, 1);
    read(file, z_msb, 1);

    // store obtained values
    *x = (__int16_t)((x_msb[0] << 8) | x_lsb[0]); // shift high byte left by 8b then cat with low byte
    *y = (__int16_t)((y_msb[0] << 8) | y_lsb[0]);
    *z = (__int16_t)((z_msb[0] << 8) | z_lsb[0]);

    // Handle negative values (2s complement)
    if (*x < 0)
        *x = ~(*x) + 1;
    if (*y < 0)
        *y = ~(*y) + 1;
    if (*z < 0)
        *z = ~(*z) + 1;
}

int main()
{
    int file;
    const char *filename = "/dev/i2c-2";

    // Open I2C bus
    if ((file = open(filename, O_RDWR)) < 0)
    {
        perror("Failed to open the I2C bus");
        return 1;
    }

    // Set the slave address
    if (ioctl(file, I2C_SLAVE, QMC5883L_ADDRESS) < 0)
    {
        perror("Failed to set I2C address");
        return 1;
    }

    // Initialize sensor
    writeRegister(file, SET_RESET_PERIOD_REGISTER, 0x01);
    writeRegister(file, CONTROL_REG1, 0x1D); // OSR = 512, Full Scale Range = 8 Gauss, ODR = 200Hz, continuous measurement mode
    writeRegister(file, CONTROL_REG2, 0x00); // Default settings

    // Read magnetic field values
    __int16_t x, y, z;
    while (1)
    {
        readMagneticField(file, &x, &y, &z);
        printf("Magnetic Field (uT) -> X: %d, Y: %d, Z: %d\n", x, y, z);
        usleep(500000); // Delay 500ms
    }

    close(file);
    return 0;
}