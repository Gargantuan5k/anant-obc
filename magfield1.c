#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define HMC_ADDR 0X1E

void read_xyz(int fd, float * ptr) {
    unsigned char buf[6];
    buf[0] = 0x03; // X MSB register
    
    if(write(fd, buf, 1) != 1) {
        printf("couldn't write to register\n");
        return -1.0;
    }

    if(read(fd, buf, 6) != 2) {
        printf("couldn't read from resgiter\n");
        return -1.0;
    }

    ptr[0] = (buf[0]<<8) | buf[1];
    ptr[1] = (buf[2]<<8) | buf[3];
    ptr[2] = (buf[4]<<8) | buf[5];
}

int main() {
    int fd = open("/dev/i2c-1", O_RDWR);
    if(fd<0) return -1;
    
    if (ioctl(fd, I2C_SLAVE, HMC_ADDR) < 0) {
        printf("Error setting slave address\n");
        return -1;
    }

    float xyz[3];
    read_xyz(fd, xyz);

    printf("%f", xyz[0]);
    printf("%f", xyz[1]);
    printf("%f", xyz[2]);


    return 0;
}