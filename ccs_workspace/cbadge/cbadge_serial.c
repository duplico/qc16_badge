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

volatile serial_header_t serial_header_in; // Needs no initialization.
serial_header_t serial_header_out; // Needs no initialization.
volatile uint8_t serial_buffer_in[SERIAL_BUFFER_LEN]; // Needs no initialization.
volatile uint8_t serial_buffer_out[SERIAL_BUFFER_LEN]; // Needs no initialization
uint8_t serial_phy_mode_ptx; // Needs no initialization.
volatile uint8_t serial_phy_state; // 0-init is fine.
volatile uint8_t serial_phy_index; // Initialized in ISR.
volatile uint8_t serial_phy_timeout_counter; // 0-init is fine.

uint16_t connected_badge_id; // initialized in code
uint8_t serial_ll_state; // initialized in code
uint16_t serial_ll_timeout_ms; // initialized in code

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
    serial_header_out.badge_type = badge_conf.badge_type;
    serial_header_out.from_id = badge_conf.badge_id;
    serial_header_out.payload_len = payload_len;
    serial_header_out.crc16_payload = crc16_buf(serial_buffer_out, payload_len);
    crc16_header_apply(&serial_header_out);
    serial_phy_state = SERIAL_PHY_STATE_IDLE;
    UCA0TXBUF = SERIAL_PHY_SYNC_WORD;
    serial_phy_timeout_counter = SERIAL_PHY_TIMEOUT_MS;
    // The interrupts will take it from here.
}

void serial_pair() {
    // Guard against clobbering anything:
    while (serial_phy_state != SERIAL_PHY_STATE_IDLE);
    pair_payload_t *pair_payload_out = (pair_payload_t *) serial_buffer_out;
    memset(pair_payload_out, 0, sizeof(pair_payload_t));
    pair_payload_out->agent_present = 1; // Always 1 because we can always do a mission.
    pair_payload_out->badge_type = badge_conf.badge_type;
    pair_payload_out->badge_id = badge_conf.badge_id;
    pair_payload_out->clock_is_set = 0;
    memcpy(&pair_payload_out->element_level[3], badge_conf.element_level, sizeof(element_type)*3);
    pair_payload_out->element_level_max[3] = 5;
    pair_payload_out->element_level_max[4] = 5;
    pair_payload_out->element_level_max[5] = 5;
    memcpy(&pair_payload_out->element_level_progress[3], badge_conf.element_level_progress, 3);
    memcpy(&pair_payload_out->element_qty[3], badge_conf.element_qty, 4*3);
    memcpy(pair_payload_out->handle, badge_conf.handle, QC16_BADGE_NAME_LEN);
    pair_payload_out->handle[QC16_BADGE_NAME_LEN] = 0x00;
    pair_payload_out->last_clock = 0;
    pair_payload_out->element_selected = badge_conf.element_selected;
    // pair_payload_out.missions are DONTCARE

    serial_send_start(SERIAL_OPCODE_PAIR, sizeof(pair_payload_t));
}

void serial_send_stats() {
    // Guard against clobbering anything:
    while (serial_phy_state != SERIAL_PHY_STATE_IDLE);
    memcpy((void *)serial_buffer_out, &badge_conf.stats, sizeof(cbadge_stats_t));
    memset((void *)serial_buffer_out, 0x00, sizeof(qbadge_stats_t)-sizeof(cbadge_stats_t));
    serial_send_start(SERIAL_OPCODE_STATA, sizeof(qbadge_stats_t));
}

void serial_element_update() {
    // Guard against clobbering anything:
    while (serial_phy_state != SERIAL_PHY_STATE_IDLE);
    memcpy((void *)serial_buffer_out, &badge_conf.element_selected, sizeof(element_type));
    serial_send_start(SERIAL_OPCODE_ELEMENT, sizeof(element_type));
}

void serial_dump_answer(uint8_t pillar_id) {
    // Guard against clobbering anything:
    while (serial_phy_state != SERIAL_PHY_STATE_IDLE);
    memcpy((void *)serial_buffer_out, &badge_conf.element_qty[pillar_id], sizeof(uint32_t));
    serial_send_start(SERIAL_OPCODE_DUMPA, sizeof(uint32_t));
    badge_conf.element_qty[pillar_id] = 0;
}

void serial_ll_timeout() {
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        if (!badge_active) {
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
        serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
        if (
                (serial_phy_mode_ptx && !(SERIAL_DIO_IN & SERIAL_DIO2_PRX))
                || (!serial_phy_mode_ptx && !(SERIAL_DIO_IN & SERIAL_DIO1_PTX))
        ) {
            // We read connection-sense LOW, so we're unplugged.
            serial_enter_prx();
            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            s_disconnected = 1;
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
    // Note: the proper operation of this function depends on an alternating
    //       RX/TX mode of operation; that is, there's a danger that if the
    //       remote badge sends multiple messages in a row, the header and
    //       payload could get clobbered.
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // Expecting a HELO.
        if (serial_header_in.opcode == SERIAL_OPCODE_HELO) {
            // Need to send an ACK.
            serial_send_start(SERIAL_OPCODE_ACK, 0);
            // Once that completes, we'll be connected.
            connected_badge_id = serial_header_in.from_id;
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
            serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
            s_connected = 1;
        }
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // We sent a HELO when we entered this state, so we need an ACK.
        if (serial_header_in.opcode == SERIAL_OPCODE_ACK) {
            connected_badge_id = serial_header_in.from_id;
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
            serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;
            s_connected = 1;
        }
        break;
    case SERIAL_LL_STATE_C_IDLE:
        // cbadges must implement the following commands from idle:
        // * STAT1Q - send a copy of badge_conf.stats
        // * STAT2Q - send a pairing message (but not pair)
        // * SETID  - ONLY FROM CONTROLLER - set ID and respond with ACK.
        // * SETNAME - Set my handle
        // * DUMPQ - Reply with a DUMPA
        // * DISCON - Simulate a physical disconnection.
        // * SETTYPE - Promote myself to uber or handler, respond with ACK. (CONTROLLER ONLY)
        // * PAIR - Begin pairing.
        serial_ll_timeout_ms = SERIAL_C_DIO_POLL_MS;

        if (serial_header_in.opcode == SERIAL_OPCODE_STAT1Q) {
            serial_send_stats();

        } else if (serial_header_in.opcode == SERIAL_OPCODE_STAT2Q) {
            serial_pair();

        } else if (serial_header_in.opcode == SERIAL_OPCODE_SETID) {
            memcpy(&badge_conf.badge_id, (uint8_t *) serial_buffer_in, sizeof(badge_conf.badge_id));
            write_conf();
            set_display_type(DISPLAY_OFF);
            serial_send_start(SERIAL_OPCODE_ACK, 0);
        } else if (serial_header_in.opcode == SERIAL_OPCODE_SETNAME) {
            memcpy(&badge_conf.handle, (uint8_t *) serial_buffer_in, QC16_BADGE_NAME_LEN);
            // Guarantee null term:
            badge_conf.handle[QC16_BADGE_NAME_LEN] = 0x00;
            write_conf();
            serial_send_start(SERIAL_OPCODE_ACK, 0);

        } else if (serial_header_in.opcode == SERIAL_OPCODE_DUMPQ) {
            uint8_t pillar_id = serial_buffer_in[0];
            if (pillar_id > 3) {
                // Invalid pillar, do nothing.
                break;
            }
            serial_dump_answer(pillar_id);

        } else if (serial_header_in.opcode == SERIAL_OPCODE_DISCON) {
            serial_enter_prx();
            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            s_disconnected = 1;

        } else if (serial_header_in.opcode == SERIAL_OPCODE_SETTYPE) {
            badge_conf.badge_type = serial_buffer_in[0] & 0b11000000;
            write_conf();
            serial_send_start(SERIAL_OPCODE_ACK, 0);

        } else if (serial_header_in.opcode == SERIAL_OPCODE_PAIR) {
            serial_ll_state = SERIAL_LL_STATE_C_PAIRED;
            s_paired = 1;
            badge_conf.element_selected = ELEMENT_COUNT_NONE;
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
            mission_t *mission = (mission_t *) &serial_buffer_in[1];
            complete_mission(mission);
        } else if (serial_header_in.opcode == SERIAL_OPCODE_SETNAME) {
            memcpy(&badge_conf.handle, (uint8_t *) serial_buffer_in, QC16_BADGE_NAME_LEN);
            // Guarantee null term:
            badge_conf.handle[QC16_BADGE_NAME_LEN] = 0x00;
            write_conf();
            // Don't ACK, but rather send a pairing update with our new handle:
            serial_pair();
        } else if (serial_header_in.opcode == SERIAL_OPCODE_ELEMENT && serial_buffer_in[0] == 123) {
            set_badge_connected(connected_badge_id);
        }
        break;
    }
}

void serial_phy_handle_rx() {
    // We just got a complete serial message
    //  (header and, possibly, payload).
    if (!validate_header_simple((serial_header_t *) &serial_header_in)) {
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
        badge_active = 0;
    } else if (badge_conf.in_service) {
        // We are under our own power.
        if (!badge_conf.activated) {
            badge_conf.activated = 1;
            // This badge was just turned on under its own power for the first time!
            s_activated = 1;
            write_conf();
        }
        badge_active = 24;
    }

    // Pause the UART peripheral:
    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (8 MHz)
    // 8N1 (8 data bits, no parity bits, 1 stop bit)
    UCA0CTLW0 |= UCSSEL__SMCLK + UCPEN_0 + UCSPB_0;
    // Configure the baud rate to 230400.
    //  (See page 589 in the family user's guide, SLAU445I)
    // The below is for 8.00 MHz SMCLK:
    UCA0BRW = 2;
    UCA0MCTLW = 0xBB00 | UCOS16_1 | UCBRF_2;

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
