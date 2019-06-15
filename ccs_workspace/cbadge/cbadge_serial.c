/*
 * serial.c
 *
 *  Created on: Jun 9, 2019
 *      Author: george
 */
#include <stdint.h>

#include <msp430fr2111.h> // TODO: device specific

#include <cbadge.h>
#include <cbadge_serial.h>

#define SERIAL_ENTER_PTX SYSCFG3 |= USCIARMP_1
#define SERIAL_ENTER_PRX SYSCFG3 &= ~USCIARMP_1

volatile serial_header_t serial_header_in = {0,};
serial_header_t serial_header_out = {0,};
volatile uint8_t *serial_header_buf;
volatile uint8_t serial_buffer[SERIAL_BUFFER_LEN] = {0,};
volatile uint8_t serial_phy_state = 0;
volatile uint8_t serial_phy_index = 0;
// TODO: rename this
volatile uint8_t serial_active_ms = 0;
volatile uint16_t serial_running_crc = 0;

uint8_t serial_ll_state;
// TODO: rename this:
uint16_t serial_ll_timeout_time;

void serial_send_start() {
    // TODO: assert serial_phy_state == SERIAL_PHY_STATE_IDLE
    serial_phy_state = SERIAL_PHY_STATE_IDLE;
    UCA0TXBUF = SERIAL_PHY_SYNC_WORD;
    // The interrupts will take it from here.
}


void serial_ll_timeout() {
    // TODO: Call this ever.
    // TODO: Is this done? ---- Note: Serial alternative switching is controlled by TBRMP in SYSCFG3
    switch(serial_ll_state) {
    case SERIAL_MODE_NC_PRX:
        if (!my_conf.active) {
            break; // If we're not active, we never leave PRX.
            // TODO: Adjust timeout.
        }
        SERIAL_ENTER_PTX;

        serial_ll_state = SERIAL_MODE_NC_PTX;
        serial_ll_timeout_time = PTX_TIME_MS;

        // TODO: Send a HELO.
        break;
    case SERIAL_MODE_NC_PTX:
        SERIAL_ENTER_PRX;

        serial_ll_state = SERIAL_MODE_NC_PRX;
        serial_ll_timeout_time = PRX_TIME_MS;
        break;
//    default:
        // TODO
    }
}

/// Call this every 1 ms
void serial_ll_ms_tick() {

    // TODO: Check GPIO
}

void serial_ll_handle_rx() {
    switch(serial_ll_state) {
    case SERIAL_MODE_NC_PRX:
        // Expecting a HELO.
        if (serial_header_in.opcode == SERIAL_OPCODE_HELO) {
            // Need to send an ACK.
            serial_header_out.from_id = my_conf.badge_id;
            serial_header_out.opcode = SERIAL_OPCODE_ACK;
            serial_header_out.payload_len = 0;
            serial_header_out.to_id = SERIAL_ID_ANY;
            serial_header_out.crc16_header = crc16_buf((uint8_t *) &serial_header_out, sizeof(serial_header_t) - sizeof(serial_header_out.crc16_header));
            // DIO2: Output HIGH.
            SERIAL_DIO_DIR |= SERIAL_DIO2_RTR;
            SERIAL_DIO_OUT |= SERIAL_DIO2_RTR;
            // DIO1: Input w/ pulldown
            SERIAL_DIO_DIR &= ~SERIAL_DIO1_ABS;
            SERIAL_DIO_REN |= SERIAL_DIO1_ABS;
            SERIAL_DIO_OUT &= ~SERIAL_DIO1_ABS;
            serial_send_start();
            // Once that completes, we'll be connected.
            serial_ll_state = SERIAL_MODE_C_IDLE;
            s_serial_ll = 1;
        }
        break;
    case SERIAL_MODE_NC_PTX:
        // We sent a HELO when we entered this state, so we need an ACK.
        if (serial_header_in.opcode == SERIAL_OPCODE_ACK) {
            // DIO1: Output HIGH.
            SERIAL_DIO_DIR |= SERIAL_DIO1_ABS;
            SERIAL_DIO_OUT |= SERIAL_DIO1_ABS;
            // DIO2: Input w/ pulldown
            SERIAL_DIO_DIR &= ~SERIAL_DIO2_RTR;
            SERIAL_DIO_REN |= SERIAL_DIO2_RTR;
            SERIAL_DIO_OUT &= ~SERIAL_DIO2_RTR;
            serial_ll_state = SERIAL_MODE_C_IDLE;
            s_serial_ll = 1;
        }
        break;
    case SERIAL_MODE_C_IDLE:
        // TODO: handle these things.
        break;
    }
}

void serial_phy_handle_rx() {
    // We just got a complete serial message
    //  (header and, possibly, payload).
    if (crc16_buf((uint8_t *) &serial_header_in, sizeof(serial_header_t) - sizeof(serial_header_in.crc16_header)) != serial_header_in.crc16_header ) {
        // Bad CRC header.
        return;
    }

    if (crc16_buf(serial_buffer, serial_header_in.payload_len) != serial_header_in.crc16_payload) {
        // Bad payload header
        // TODO: consider NACKing if we're connected?
        return;
    }

    // Payload len has already been validated.

    // TODO: check from ID

    // TODO: check to ID???

    // TODO: check for valid opcode

    // So, clear out our timeout:
    serial_active_ms = SERIAL_TIMEOUT_TICKS;
    // Handle the message at the link-layer.
    serial_ll_handle_rx();
}

void init_serial() {
    // Our initial config is in PRX mode. We'll stay there,
    //  unless we're ACTIVE.
    serial_ll_state = SERIAL_MODE_NC_PRX;

    // Read P1.1 to determine whether we're free-standing, or
    //  if we're connected to a badge that's powering us. If it's LOW,
    //  then this badge is active WITHOUT another badge powering us.
    //  How cool! Switch that pin to an output to signal that
    //  this badge is now active.
    if (!(P1IN & BIT1)) {
        // We are under our own power.
        // Assert the Active Badge Signal (ABS, P1.1).
        P1DIR |= BIT1;
        P1OUT |= BIT1;
        if (!my_conf.activated) {
            my_conf.activated = 1;
            // This badge was just turned on under its own power for the first time!
            s_activated = 1;
        }
        my_conf.active = 1;
    } else {
        my_conf.active = 0;
    }

    // Pause the UART peripheral:
    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (~1 MHz)
    // 8E2 (Enable parity, even parity, 2 stop bits)
    UCA0CTLW0 |= UCSSEL__SMCLK + UCPEN_1 + UCPAR__EVEN + UCSPB_1;
    // Configure the baud rate to 9600.
    //  (See page 589 in the family user's guide, SLAU445I)
    // The below is for 1.00 MHz SMCLK:
    UCA0BR0 = 6;
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x2000 | UCOS16 | UCBRF_8;

    // Activate the UART:
    UCA0CTLW0 &= ~UCSWRST;

    // The TX interrupt flag (UCTXIFG) gets set upon enabling the UART.
    //  But, we'd prefer that interrupt not to fire, so we'll clear it
    //  now:
    UCA0IFG &= ~UCTXIFG;

    // Enable interrupts for TX and RX:
    UCA0IE |= UCTXIE | UCRXIE;
}

#pragma vector=USCI_A0_VECTOR
__interrupt void serial_isr() {
    switch(__even_in_range(UCA0IV, UCIV__UCTXIFG)) {
    case UCIV__UCRXIFG:
        // Receive buffer full; a byte is ready to read.
        switch(serial_phy_state) {
        case SERIAL_PHY_STATE_IDLE:
            // Initiation of a reception. This needs to be the SYNC WORD.
            if (UCA0RXBUF == SERIAL_PHY_SYNC_WORD) {
                serial_phy_state = SERIAL_PHY_STATE_RX_HEADER;
                serial_phy_index = 0;
            }
            break;
        case SERIAL_PHY_STATE_RX_HEADER:
            ((uint8_t *) (&serial_header_in))[serial_phy_index] = UCA0RXBUF;
            serial_phy_index++;
            if (serial_phy_index == sizeof(serial_header_in)) {
                // Header is complete.
                if (serial_header_in.payload_len > SERIAL_BUFFER_LEN) {
                    // If the length is longer than we can handle,
                    //  we're just going to ignore it.
                    serial_phy_state = SERIAL_PHY_STATE_IDLE;
                } else if (serial_header_in.payload_len == 0) {
                    // No payload; we're done.
                    serial_phy_state = SERIAL_PHY_STATE_IDLE;
                    f_serial_phy = SERIAL_RX_DONE;
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
            serial_phy_index++;
            if (serial_phy_index == serial_header_in.payload_len) {
                serial_phy_state = SERIAL_PHY_STATE_IDLE;
                serial_phy_index = 0;
                // Done.
                f_serial_phy = SERIAL_RX_DONE;
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
                f_serial_phy = SERIAL_TX_DONE;
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
