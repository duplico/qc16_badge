/*
 * serial.c
 *
 *  Created on: Jun 9, 2019
 *      Author: george
 */
#include <stdint.h>
#include <string.h>

#include <msp430fr2111.h>

#include <cbadge.h>
#include <cbadge_serial.h>

volatile serial_header_t serial_header_in = {0,};
serial_header_t serial_header_out = {0,};
volatile uint8_t *serial_header_buf;
volatile uint8_t serial_buffer_in[SERIAL_BUFFER_LEN] = {0,};
volatile uint8_t serial_buffer_out[SERIAL_BUFFER_LEN] = {0,};
uint8_t serial_phy_mode_ptx = 0;
volatile uint8_t serial_phy_state = 0;
volatile uint8_t serial_phy_index = 0;
volatile uint8_t serial_phy_timeout_counter = 0;

uint16_t connected_badge_id;

uint8_t serial_ll_state;
uint16_t serial_ll_timeout_ms;

void serial_enter_ptx() {
    // Swap to the alternate/PTX USCI config:
    SYSCFG3 |= USCIARMP_1;
    // Reconfigure our DIO for PTX:
    //  DIO1(PTX) is a HIGH output
    SERIAL_DIO_DIR |= SERIAL_DIO1_PTX;
    SERIAL_DIO_OUT |= SERIAL_DIO1_PTX;
    //  DIO2(PRX) is a LOW-pulled input
    SERIAL_DIO_DIR &= ~SERIAL_DIO2_PRX;
    SERIAL_DIO_REN |= SERIAL_DIO2_PRX;
    SERIAL_DIO_OUT &= ~SERIAL_DIO2_PRX;


    // PHY and link-layer state:
    serial_phy_mode_ptx=1;
    serial_ll_timeout_ms = PTX_TIME_MS;
}

void serial_enter_prx() {
    // Swap to the main/PRX USCI config:
     SYSCFG3 &= ~USCIARMP_1;

     // Reconfigure our DIO for PRX:
     //  DIO2(PRX) is a HIGH output
     SERIAL_DIO_DIR |= SERIAL_DIO2_PRX;
     SERIAL_DIO_OUT |= SERIAL_DIO2_PRX;
     //  DIO1(PTX) is a LOW-pulled input
     SERIAL_DIO_DIR &= ~SERIAL_DIO1_PTX;
     SERIAL_DIO_REN |= SERIAL_DIO1_PTX;
     SERIAL_DIO_OUT &= ~SERIAL_DIO1_PTX;

    // PHY and link-layer state:
    serial_phy_mode_ptx=0;
    serial_ll_timeout_ms = PRX_TIME_MS;
}

/// Send a message, applying the payload, len, crc, and from-ID.
void serial_send_start(uint8_t opcode, uint8_t payload_len) {
    // Block until the PHY is idle.
    while (serial_phy_state != SERIAL_PHY_STATE_IDLE);
    serial_header_out.opcode = opcode;
    serial_header_out.to_id = SERIAL_ID_ANY;
    serial_header_out.from_id = badge_conf.badge_id;
    serial_header_out.payload_len = payload_len;
    serial_header_out.crc16_payload = crc16_buf(serial_buffer_out, payload_len);
    crc16_header_apply(&serial_header_out);
    serial_phy_state = SERIAL_PHY_STATE_IDLE;
    UCA0TXBUF = SERIAL_PHY_SYNC_WORD;
    // The interrupts will take it from here.
}

void serial_pair() {
    pair_payload_t *pair_payload_out = (pair_payload_t *) serial_buffer_out;
    pair_payload_out->agent_present = 0;
    pair_payload_out->badge_type = badge_conf.badge_type;
    pair_payload_out->clock_is_set = 0;
    memcpy(&pair_payload_out->element_level[3], badge_conf.element_level, sizeof(element_type)*3);
    pair_payload_out->element_level_max[3] = 5;
    pair_payload_out->element_level_max[4] = 5;
    pair_payload_out->element_level_max[5] = 5;
    memcpy(&pair_payload_out->element_level_progress[3], badge_conf.element_level_progress, 3);
    memcpy(&pair_payload_out->element_qty[3], badge_conf.element_qty, 4);
    memcpy(pair_payload_out->handle, badge_conf.handle, QC16_BADGE_NAME_LEN);
    pair_payload_out->handle[QC16_BADGE_NAME_LEN-1] = 0x00;
    pair_payload_out->last_clock = 0;
    // pair_payload_out.missions are DONTCARE

    serial_send_start(SERIAL_OPCODE_PAIR, sizeof(pair_payload_t));
}

void serial_ll_timeout() {
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        if (!badge_conf.active) {
            serial_ll_timeout_ms = PRX_TIME_MS;
            break; // If we're not active, we never leave PRX.
        }

        // Pin us in PRX mode if the PTX input is asserted.
        if (SERIAL_DIO_IN & SERIAL_DIO1_PTX) {
            serial_ll_timeout_ms = PRX_TIME_MS;
            break;
        }

        serial_enter_ptx();

        serial_ll_state = SERIAL_LL_STATE_NC_PTX;
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // Pin us in PTX mode if the PRX input is asserted.
        if (SERIAL_DIO_IN & SERIAL_DIO2_PRX) {
            serial_ll_timeout_ms = PTX_TIME_MS;

            serial_send_start(SERIAL_OPCODE_HELO, 0);
            break;
        }

        serial_enter_prx();

        serial_ll_state = SERIAL_LL_STATE_NC_PRX;
        break;
    default:
        if (
                (serial_phy_mode_ptx && !(SERIAL_DIO_IN & SERIAL_DIO2_PRX))
                || (!serial_phy_mode_ptx && !(SERIAL_DIO_IN & SERIAL_DIO1_PTX))
        ) {
            // TODO: Flag a disconnection.
            // We read connection-sense LOW, so we're unplugged.
            serial_enter_prx();
            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            // TODO: send a flag to the event loop.
        } else {
            serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
        }
        break;
    }
}

/// Call this every 1 ms
void serial_ll_ms_tick() {
    serial_ll_timeout_ms--;

    if (!serial_ll_timeout_ms) {
        serial_ll_timeout();
    }
}

void serial_ll_handle_rx() {
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // Expecting a HELO.
        if (serial_header_in.opcode == SERIAL_OPCODE_HELO) {
            // Need to send an ACK.
            serial_send_start(SERIAL_OPCODE_ACK, 0);
            // Once that completes, we'll be connected.
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
            serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
            s_connected = 1;
        }
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // We sent a HELO when we entered this state, so we need an ACK.
        if (serial_header_in.opcode == SERIAL_OPCODE_ACK) {
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
            serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
            s_connected = 1;
        }
        break;
    case SERIAL_LL_STATE_C_IDLE:
        serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
        if (serial_header_in.opcode == SERIAL_OPCODE_DISCON) {
            serial_enter_prx();
            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
        } else if (serial_header_in.opcode == SERIAL_OPCODE_PAIR) {
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            s_paired = 1;
            serial_pair();
        }
        break;
    case SERIAL_LL_STATE_C_PAIRING:
        if (serial_header_in.opcode == SERIAL_OPCODE_PAIR) {
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            s_paired = 1;
        }
        break;
    case SERIAL_LL_STATE_C_PAIRED:
        // The element selection buttons are ignored.
        // Color-picking buttons are ignored.
        // But, mission-doing is a thing!
        if (serial_header_in.opcode == SERIAL_OPCODE_GOMISSION) {
            mission_t *mission = &serial_buffer_in[1];
        }
        break;
    }
}

void serial_phy_handle_rx() {
    // We just got a complete serial message
    //  (header and, possibly, payload).
    if (!validate_header(&serial_header_in)) {
        return;
    }

    // Payload len has already been validated.

    // So, clear out our timeout:
    serial_phy_timeout_counter = SERIAL_PHY_TIMEOUT_MS;
    // Handle the message at the link-layer.
    serial_ll_handle_rx();
}

void init_serial() {
    // Read our serial DIO lines to determine whether we're free-standing, or
    //  if we're connected to a badge that's powering us.
    // If both DIO lines are LOW, that means this badge is active,
    //  WITHOUT another badge powering us.
    if (SERIAL_DIO_IN & (SERIAL_DIO1_PTX | SERIAL_DIO2_PRX)) {
        // We are being externally powered, because at least one of
        //  DIO1_PTX and DIO2_PRX are asserted (and we have those
        //  set as inputs with pull-down resistors)
        badge_conf.active = 0;
    } else if (badge_conf.badge_id != CBADGE_ID_MAX_UNASSIGNED) {
        // We are under our own power.
        if (!badge_conf.activated) {
            badge_conf.activated = 1;
            // This badge was just turned on under its own power for the first time!
            s_activated = 1;
        }
        badge_conf.active = 1;
    }

    // Pause the UART peripheral:
    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (~1 MHz)
    // 8E2 (Enable parity, even parity, 2 stop bits)
//    UCA0CTLW0 |= UCSSEL__SMCLK + UCPEN_1 + UCPAR__EVEN + UCSPB_1;
    UCA0CTLW0 |= UCSSEL__SMCLK + UCPEN_0 + UCSPB_0;
    // Configure the baud rate to 38400.
    //  (See page 589 in the family user's guide, SLAU445I)
    // The below is for 1.00 MHz SMCLK:
    UCA0BR0 = 1;
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x0000 | UCOS16 | UCBRF_10;

    // Activate the UART:
    UCA0CTLW0 &= ~UCSWRST;

    // The TX interrupt flag (UCTXIFG) gets set upon enabling the UART.
    //  But, we'd prefer that interrupt not to fire, so we'll clear it
    //  now:
    UCA0IFG &= ~UCTXIFG;

    // Our initial config is in PRX mode.
    serial_ll_state = SERIAL_LL_STATE_NC_PRX;
    serial_enter_prx();

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
            serial_buffer_in[serial_phy_index] = UCA0RXBUF;
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
        // Transmit buffer full, ready to load another byte to send.
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
                UCA0TXBUF = serial_buffer_out[serial_phy_index];
                serial_phy_index++;
            }
            break;
        default:
            break;
        }
        break;
    }

}
