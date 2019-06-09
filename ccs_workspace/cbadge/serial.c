/*
 * serial.c
 *
 *  Created on: Jun 9, 2019
 *      Author: george
 */
#include <stdint.h>

#include <msp430.h> // TODO: device specific

#include <cbadge.h>
#include "serial.h"

volatile serial_header_t serial_header = {0,};
volatile uint8_t *serial_header_buf;
volatile uint8_t serial_buffer[SERIAL_BUFFER_LEN] = {0,};
volatile uint8_t serial_phy_state = 0;
volatile uint8_t serial_phy_index = 0;

// TODO: Add the ability to time out.

#pragma vector=USCI_A0_VECTOR
__interrupt void serial_isr() {
    // TODO: If a higher-number (lower priority) interrupt is unused,
    //       reduce the second parameter below:
    switch(__even_in_range(UCA0IV, 0x08)) {
    case UCIV__UCRXIFG:
        // Receive buffer full; a byte is ready to read.
        switch(serial_phy_state) {
        case SERIAL_PHY_STATE_TX_HEADER:
            // We just finished sending header byte index serial_phy_index.
            serial_phy_index++;
            if (serial_phy_index < sizeof(serial_header)) {
                // The header isn't finished sending yet.
                // TODO: Confirm that this serializes properly between platforms:
                UCA0TXBUF = ((uint8_t *) (&serial_header))[serial_phy_index];
                break; // done; don't fall through.
            } else {
                // Header is completely sent.
                serial_phy_state = SERIAL_PHY_STATE_TX_PAYLOAD;
                serial_phy_index = 0;
                // Fall through, and let the logic below handle this...
            }
        case SERIAL_PHY_STATE_TX_PAYLOAD:
            // This works slightly differently than the receiving version.
            //  It's time to TRANSMIT serial_buffer[serial_phy_index]
            //  (it hasn't already been done).
            //  That is, we just transmitted serial_buffer[serial_phy_index-1]
            //  (or, if serial_phy_index == 0, we just finished sending a header)

            if (serial_phy_index == serial_header.payload_len) {
                // Done sending.
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
                serial_phy_index = 0;
                // Done.
                // TODO: check CRC (but not here)
                f_serial = SERIAL_TX_DONE;
                LPM3_EXIT;
            } else {
                // Need to send another.
                UCA0TXBUF = serial_buffer[serial_phy_index];
                serial_phy_index++;
            }
            break;
        }
        break;
    case UCIV__UCTXIFG:
        // Transmit buffer full' ready to load another byte to send.
        switch(serial_phy_state) {
        case SERIAL_PHY_STATE_TX_HEADER:
            // We just finished sending header byte index serial_phy_index.
            serial_phy_index++;
            if (serial_phy_index < sizeof(serial_header)) {
                // The header isn't finished sending yet.
                // TODO: Confirm that this serializes properly between platforms:
                UCA0TXBUF = ((uint8_t *) (&serial_header))[serial_phy_index];
                break; // done; don't fall through.
            } else {
                // Header is completely sent.
                serial_phy_state = SERIAL_PHY_STATE_TX_PAYLOAD;
                serial_phy_index = 0;
                // Fall through, and let the logic below handle this...
            }
        case SERIAL_PHY_STATE_TX_PAYLOAD:
            // This works slightly differently than the receiving version.
            //  It's time to TRANSMIT serial_buffer[serial_phy_index]
            //  (it hasn't already been done).
            //  That is, we just transmitted serial_buffer[serial_phy_index-1]
            //  (or, if serial_phy_index == 0, we just finished sending a header)

            if (serial_phy_index == serial_header.payload_len) {
                // Done sending.
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
                serial_phy_index = 0;
                // Done.
                // TODO: check CRC (but not here)
                f_serial = SERIAL_TX_DONE;
                LPM3_EXIT;
            } else {
                // Need to send another.
                UCA0TXBUF = serial_buffer[serial_phy_index];
                serial_phy_index++;
            }
            break;
        }
        break;
    case UCIV__UCSTTIFG:
        // Start bit received; a message is being sent to us.
        break;
    case UCIV__UCTXCPTIFG:
        // We have completed transmitting a byte.
        break;
    }

}
