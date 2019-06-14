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

volatile serial_header_t serial_header_in = {0,};
serial_header_t serial_header_out = {0,};
volatile uint8_t *serial_header_buf;
volatile uint8_t serial_buffer[SERIAL_BUFFER_LEN] = {0,};
volatile uint8_t serial_phy_state = 0;
volatile uint8_t serial_phy_index = 0;
volatile uint8_t serial_active_ticks = 0;
volatile uint16_t serial_running_crc = 0;

uint8_t serial_ll_state;

void serial_send_start() {
    // TODO: assert serial_phy_state == SERIAL_PHY_STATE_IDLE
    serial_phy_state = SERIAL_PHY_STATE_IDLE;
    UCA0TXBUF = SERIAL_PHY_SYNC_WORD;
    // The interrupts will take it from here.
}

void serial_handle_rx() {
    // We just got a complete serial message
    //  (header and, possibly, payload).
    switch(serial_header_in.opcode) {
    case SERIAL_OPCODE_HELO:
        // Need to send an ACK.
        serial_header_out.from_id = my_conf.badge_id;
        serial_header_out.opcode = SERIAL_OPCODE_ACK;
        serial_header_out.payload_len = 0;
        serial_header_out.to_id = SERIAL_ID_ANY;
        serial_send_start();
        // Once that completes, we'll be connected.
        break;
    case SERIAL_OPCODE_ACK:
        // Connection done, what's next?
        break;
    }
}

void init_serial() {
    // Our initial config is in PRX mode. This is how we want to stay,
    //  unless we are ACTIVE. If we're active (TODO), we should
    //  occasionally use `SYSCFG3 |= USCIARMP_1` to swap to PTX.
    serial_ll_state = SERIAL_MODE_NC_PRX;

    // TODO: If we source the UART from ACLK we can get better sleep mode.
    // Pause the UART peripheral:
    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (~1 MHz)
    // 8E2 (Enable parity, even parity, 2 stop bits)
    UCA0CTLW0 |= UCSSEL__SMCLK + UCPEN_1 + UCPAR__EVEN + UCSPB_1;
    // Configure the baud rate to 9600.
    //  (See page 589 in the family user's guide, SLAU445I)
//    UCA0BR0 = 6; // 1048576/9600/16
//    UCA0BR1 = 0x00;
//    UCA0MCTLW = 0x2200 | UCOS16 | UCBRF_13;
    // The below is for 1 MHz:
    UCA0BR0 = 6;
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x2000 | UCOS16 | UCBRF_8;

    // Activate the UART:
    UCA0CTLW0 &= ~UCSWRST;

    // TODO: clear that initial TXIV

    UCA0IE |= UCTXIE | UCRXIE;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void serial_isr() {
    // TODO: If a higher-number (lower priority) interrupt is unused,
    //       reduce the second parameter below:
    switch(__even_in_range(UCA0IV, UCIV__UCTXIFG)) {
    case UCIV__UCRXIFG:
        // Receive buffer full; a byte is ready to read.
        switch(serial_phy_state) {
        case SERIAL_PHY_STATE_IDLE:
            // Initiation of a reception. This needs to be the SYNC WORD.
            if (UCA0RXBUF == SERIAL_PHY_SYNC_WORD) {
                serial_phy_state = SERIAL_PHY_STATE_RX_HEADER;
                serial_phy_index = 0;
                serial_active_ticks = SERIAL_TIMEOUT_TICKS;
            }
            break;
        case SERIAL_PHY_STATE_RX_HEADER:
            // TODO: Confirm that this deserializes properly between platforms:
            ((uint8_t *) (&serial_header_in))[serial_phy_index] = UCA0RXBUF;
            // TODO: how much of the header do we crc on???
            serial_running_crc = crc_build(((uint8_t *) (&serial_header_in))[serial_phy_index], !serial_phy_index);
            serial_phy_index++;
            if (serial_phy_index == sizeof(serial_header_in)) {
                // Header is complete.
                // TODO: Validate the header
                // TODO: parse the header.
                if (serial_header_in.payload_len > SERIAL_BUFFER_LEN) {
                    // If the length is longer than we can handle,
                    //  we're just going to ignore it.
                    serial_phy_state = SERIAL_PHY_STATE_IDLE;
                } else if (serial_header_in.payload_len == 0) {
                    // No payload; we're done.
                    serial_phy_state = SERIAL_PHY_STATE_IDLE;
                    f_serial = SERIAL_RX_DONE;
                    LPM3_EXIT;
                } else {
                    // There will be a payload. Start waiting for that.
                    serial_phy_state = SERIAL_PHY_STATE_RX_PAYLOAD;
                    serial_phy_index = 0;
                }
            }
            break;
        case SERIAL_PHY_STATE_RX_PAYLOAD:
            serial_buffer[serial_phy_index] = UCA0RXBUF;
            serial_running_crc = crc_build(serial_buffer[serial_phy_index], 0);
            serial_phy_index++;
            if (serial_phy_index == serial_header_in.payload_len) {
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
                serial_phy_index = 0;
                // Done.
                f_serial = SERIAL_RX_DONE;
                LPM3_EXIT;
            }
            break;
        }
        break;
    case UCIV__UCTXIFG:
        // Transmit buffer full' ready to load another byte to send.
        switch(serial_phy_state) {
        case SERIAL_PHY_STATE_IDLE:
            // We just sent a sync byte. Time to send the header:
            serial_phy_state = SERIAL_PHY_STATE_TX_HEADER;
            serial_phy_index = 0;
            // fall through...
        case SERIAL_PHY_STATE_TX_HEADER:
            // This works slightly differently than the receiving version.
            //  It's time to TRANSMIT serial_header_out[serial_phy_index]
            //  because we just sent serial_header_out[serial_phy_index-1]
            //  (or, if serial_phy_index == 0, it was the syncbyte)

            if (serial_phy_index < sizeof(serial_header_out)) {
                // Need to send another.
                UCA0TXBUF = ((uint8_t *) (&serial_header_out))[serial_phy_index];
                serial_phy_index++;
                break; // Don't fall through; we need to stay in this case.
            } else {
                // Done sending the header.
                serial_phy_state = SERIAL_PHY_STATE_TX_PAYLOAD;
                serial_phy_index = 0;
                // Fall through, and let the logic below handle this...
            }
        case SERIAL_PHY_STATE_TX_PAYLOAD:
            // This works slightly differently than the receiving version.
            //  It's time to TRANSMIT serial_buffer[serial_phy_index]
            //  because we just sent serial_buffer[serial_phy_index-1]
            //  (or, if serial_phy_index == 0, it was the header)

            if (serial_phy_index == serial_header_out.payload_len) {
                // Done sending.
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
                serial_phy_index = 0;
                // Done.
                f_serial = SERIAL_TX_DONE;
                LPM3_EXIT;
            } else {
                // Need to send another.
                UCA0TXBUF = serial_buffer[serial_phy_index];
                serial_phy_index++;
            }
            break;
        default:
            break;
        }
        break;
    }

}
