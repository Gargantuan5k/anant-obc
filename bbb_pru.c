// spi_slave_pru0.c

#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"

volatile register uint32_t __R30; // Output register
volatile register uint32_t __R31; // Input register

// Pin bit masks (adjust as per actual wiring)
#define MOSI (1 << 15) // PRU input pin for MOSI
#define MISO (1 << 14) // PRU output pin for MISO
#define SCLK (1 << 13) // PRU input pin for SCLK
#define CS (1 << 12)   // PRU input pin for Chip Select

void main(void)
{
    uint8_t received = 0;
    uint8_t to_send = 0x5A; // Example byte to send back
    int i;

    // Clear SYSCFG[STANDBY_INIT] to enable OCP master port
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    while (1)
    {
        // Wait until CS is LOW (active)
        while (__R31 & CS)
            ;

        received = 0;

        for (i = 0; i < 8; i++)
        {
            // Wait for rising edge of SCLK
            while (!(__R31 & SCLK))
                ;

            // Shift in bit from MOSI
            received <<= 1;
            if (__R31 & MOSI)
                received |= 1;

            // Output next bit on MISO
            if (to_send & (1 << (7 - i)))
                __R30 |= MISO;
            else
                __R30 &= ~MISO;

            // Wait for falling edge of SCLK
            while (__R31 & SCLK)
                ;
        }

        // You can do something with `received` here
        to_send = received; // Echo received data in next response
    }
}
