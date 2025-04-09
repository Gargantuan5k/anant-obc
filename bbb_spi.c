#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#define MODE SPI_MODE_0
#define BITS 8
#define CLK_SPD 500000
// #define LEN 1
#define DEVICE "/dev/spidev1.0"

int main(void)
{
    uint8_t tx[32] = {69};
    uint8_t rx[32] = {0};
    int fd;
    uint32_t messenger;

    struct spi_ioc_transfer trx_obj;

    // setup spi trx object
    trx_obj.tx_buf = (unsigned long)tx;
    trx_obj.rx_buf = (unsigned long)rx;
    trx_obj.speed_hz = CLK_SPD;
    trx_obj.bits_per_word = BITS;
    trx_obj.delay_usecs = 0;
    trx_obj.len = 32;

    printf("SPI Loopback with data: %d\n", tx[0]);
    printf("Initial value of rx buffer: %d\n------\n", rx[0]);

    if ((fd = open(DEVICE, O_RDWR)) < 0)
    {
        perror("SPI open");
        exit(EXIT_FAILURE);
    }

    if ((ioctl(fd, SPI_IOC_RD_MODE32, &messenger)) == -1)
    {
        perror("spi mode read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    messenger |= MODE;

    if ((ioctl(fd, SPI_IOC_WR_MODE32, &messenger)) == -1)
    {
        perror("spi mode write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if ((ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &messenger)) == -1)
    {
        perror("spi max speed read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    messenger = CLK_SPD;

    if ((ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &messenger)) == -1)
    {
        perror("spi max speed write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if ((ioctl(fd, SPI_IOC_MESSAGE(1), &trx_obj)) == -1)
    {
        perror("Nonzero return value during SPI comm");
        exit(EXIT_FAILURE);
    }

    printf("Final contents of rx buffer: %d\n", rx[0]);   

    close(fd);
    return 0;
}
