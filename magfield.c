#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#define QMC5883L_ADDRESS 0x0D
#define CONTROL_REG1 0x09 // operational modes (MODE). output data update rate (ODR), magnetic field measurement range or sensitivity of the sensors (RNG) and over sampling rate (OSR)
#define CONTROL_REG2 0x0A // Interrupt Pin enabling (INT_ENB), Point roll over function enabling(POL_PNT) and soft reset (SOFT_RST)
#define DATA_REGISTER 0x00 // read magnetic field values 
#define STATUS_REGISTER 0x06
#define SET_RESET_PERIOD_REGISTER 0x0b // recommended as per datasheet

void writeRegister(int file, unsigned char reg, unsigned char value)
{
    unsigned char buffer[2] = {reg, value}; // reg to write to, val to be written
    write(file, buffer, 2);
}

void readMagneticField(int file, int *x, int *y, int *z)
{
    unsigned char data_buffer[6];
    unsigned char status_buf[6];

    // Read from STATUS_REGISTER to ensure we're ready
    write(file, STATUS_REGISTER, 1);
    read(file, status_buf, 6);
    if (status_buf[0]) // 0x06[0] = 1 means ready
    {
        write(file, DATA_REGISTER, 1); // now read from data reg
        read(file, data_buffer, 6);
    }

    *x = (data_buffer[1] << 8) | data_buffer[0]; // shift high byte left by 8b then cat with low byte
    *y = (data_buffer[3] << 8) | data_buffer[2];
    *z = (data_buffer[5] << 8) | data_buffer[4];

    // Handle negative values (2s complement)
    if (*x > 32767)
        *x -= 65536;
    if (*y > 32767)
        *y -= 65536;
    if (*z > 32767)
        *z -= 65536;
}

int main()
{
    int file;
    const char *filename = "/dev/i2c-1";

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
    int x, y, z;
    while (1)
    {
        readMagneticField(file, &x, &y, &z);
        printf("Magnetic Field (uT) -> X: %d, Y: %d, Z: %d\n", x, y, z);
        usleep(500000); // Delay 500ms
    }

    close(file);
    return 0;
}