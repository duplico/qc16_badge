/*
 * qbadge_serial.c
 *
 *  Created on: Jun 12, 2019
 *      Author: george
 */
#include <string.h>
#include <stdio.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include <spiffs.h>

#include <qc16.h>

#include "board.h"
#include <qc16_serial_common.h>
#include <queercon_drivers/qbadge_serial.h>
#include <queercon_drivers/storage.h>
#include <ui/leds.h>

#define SERIAL_STACKSIZE 1024
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];
UART_Handle uart;
UART_Params uart_params;

PIN_Handle serial_pin_h;
PIN_State serial_pin_state;

uint8_t serial_phy_mode_ptx = 0;

uint8_t serial_ll_state;
uint32_t serial_ll_next_timeout;
Clock_Handle serial_timeout_clock_h;

spiffs_file serial_fd;

const PIN_Config serial_gpio_prx[] = {
    QC16_PIN_SERIAL_DIO1_PTX | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_SERIAL_DIO2_PRX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    PIN_TERMINATE
};

const PIN_Config serial_gpio_ptx[] = {
    QC16_PIN_SERIAL_DIO1_PTX | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_SERIAL_DIO2_PRX | PIN_INPUT_EN | PIN_PULLDOWN,
    PIN_TERMINATE
};

void serial_send_helo(UART_Handle uart) {
    serial_header_t header_out;
    header_out.from_id = 1;
    header_out.opcode = SERIAL_OPCODE_HELO;
    header_out.payload_len = 0;
    header_out.to_id = SERIAL_ID_ANY;
    header_out.crc16_payload = 0xabcd;
    crc16_header_apply(&header_out);
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(uart, &syncword, 1);
    UART_write(uart, (uint8_t *)(&header_out), sizeof(serial_header_t));
}

void serial_send_ack() {
    serial_header_t header_out;
    header_out.from_id = 1;
    header_out.opcode = SERIAL_OPCODE_ACK;
    header_out.payload_len = 0;
    header_out.to_id = SERIAL_ID_ANY;
    header_out.crc16_payload = 0xabcd;
    crc16_header_apply(&header_out);
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(uart, &syncword, 1);
    UART_write(uart, (uint8_t *)(&header_out), sizeof(serial_header_t));
}


void serial_clock_swi(UArg a0) {

}

// The UART may NOT be open when this is called.
void serial_enter_ptx() {
    serial_phy_mode_ptx = 1;
    uart_params.readTimeout = PTX_TIME_MS * 100;
    serial_ll_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
    uart = UART_open(QC16_UART_PTX, &uart_params);

    // Set the GPIO/PIN configuration to the PTX setup:
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx[0]);
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx[1]);
}

// The UART may NOT be open when this is called.
void serial_enter_prx() {
    serial_phy_mode_ptx = 0;
    uart_params.readTimeout = PRX_TIME_MS * 100;
    serial_ll_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
    uart = UART_open(QC16_UART_PRX, &uart_params);

    // Set the GPIO/PIN configuration to the PTX setup:
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx[0]);
    PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx[1]);
}

void serial_enter_c_idle() {
    serial_ll_next_timeout = Clock_getTicks() + (SERIAL_C_DIO_POLL_MS * 100);
}

void serial_rx_done(serial_header_t *header, uint8_t *payload) {
    // If this is called, it's already been validated.
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // We are expecting a HELO.
        if (header->opcode == SERIAL_OPCODE_HELO) {
            // Send an ACK, set connected.
            serial_send_ack();

            serial_enter_c_idle();
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
        }
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // We are expecting an ACK.
        if (header->opcode == SERIAL_OPCODE_ACK) {
            serial_enter_c_idle();
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
        }
        break;
    case SERIAL_LL_STATE_C_IDLE:
        if (header->opcode == SERIAL_OPCODE_PUTFILE) {
            // We're putting a file!
            // Assure that there is a null terminator in payload.
            payload[header->payload_len-1] = 0;
            if (strncmp("/photos/", payload, 8)
                    && strncmp("/colors/", payload, 8)
                    && header->from_id != CONTROLLER_ID) {
                // If this isn't in /photos/ or /colors/,
                //  and it's not from the controller, which is the only
                //  thing allowed to send us other files...
                break;
                // ignore it. no ack.
            }

            // Check to see if we would be clobbering a file, and if so,
            //   append numbers to it until we won't be.
            // (we give up once we get to 99)
            char fname[SPIFFS_OBJ_NAME_LEN] = {0,};
            strncpy(fname, payload, header->payload_len);

            uint8_t append=1;
            spiffs_stat stat;
            while (!SPIFFS_stat(&fs, fname, &stat) && append < 99) {
                sprintf(&fname[header->payload_len]-1, "%d", append++);
            }

            serial_fd = SPIFFS_open(&fs, fname, SPIFFS_O_CREAT | SPIFFS_O_WRONLY, 0);
            if (serial_fd > 0) {
                // The open worked properly...
                serial_ll_state = SERIAL_LL_STATE_C_FILE_RX;
                serial_send_ack();
            } else {
            }
        }

        if (header->opcode == SERIAL_OPCODE_DISCON) {
            if (serial_phy_mode_ptx) {
                serial_ll_state = SERIAL_LL_STATE_NC_PTX;
            } else {
                serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            }
        }
        break;
    case SERIAL_LL_STATE_C_FILE_RX:
        if (header->opcode == SERIAL_OPCODE_ENDFILE) {
            // Save the file.
            SPIFFS_close(&fs, serial_fd);
            serial_send_ack();
            serial_ll_state = SERIAL_LL_STATE_C_IDLE;
        } else if (header->opcode == SERIAL_OPCODE_APPFILE) {
            if (SPIFFS_write(&fs, serial_fd, payload, header->payload_len) == header->payload_len) {
                serial_send_ack();
            } else {
            }
        }
//    default:
    }
}

void serial_timeout() {
    volatile uint8_t i;
    switch(serial_ll_state) {
    case SERIAL_LL_STATE_NC_PRX:
        // Pin us in PRX mode if we're plugged into a PTX device.
        // (We're plugged in if we're PRX and DIO1 is high)
        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO1_PTX)) {
            // Don't timeout.
            serial_ll_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
            break;
        }
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_enter_ptx();
        serial_ll_state = SERIAL_LL_STATE_NC_PTX;
        // The UART RX doesn't turn on until we call for a read,
        //  so in order to make sure we receive a response, we need
        //  to call UART_read prior to sending our HELO message.
        // This will either return gibberish (if we're unplugged),
        //  or it will time out after PTX_TIME_MS ms.
        UART_read(uart, &i, 1);
        // NB: The next timeout, once we're pinned, will take care of the HELO.
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // Pin us in PTX mode if we're plugged into a PRX device.
        if (PIN_getInputValue(QC16_PIN_SERIAL_DIO2_PRX)) {
            // Don't timeout.
            serial_ll_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
            // Re-send our HELO:
            serial_send_helo(uart);
            break;
        }
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_enter_prx();
        serial_ll_state = SERIAL_LL_STATE_NC_PRX;
        break;
    default:
        serial_ll_next_timeout = Clock_getTicks() + (SERIAL_C_DIO_POLL_MS * 100);
        if (
                 (serial_phy_mode_ptx && !PIN_getInputValue(QC16_PIN_SERIAL_DIO2_PRX))
             || (!serial_phy_mode_ptx && !PIN_getInputValue(QC16_PIN_SERIAL_DIO1_PTX))
        ) {
            // We just registered a PHY disconnect signal.
            serial_ll_state = SERIAL_LL_STATE_NC_PRX;
            UART_close(uart);
            serial_enter_prx();
        }
        break;
//    default:
    }
}

void serial_task_fn(UArg a0, UArg a1) {
    // There are two serial modes:
    //  Primary RX - in which we listen for a HELO message, and
    //  Primary TX - in which we send a HELO and wait, very briefly, for ACK.
    serial_header_t header_in;
    uint8_t input[32];
    volatile int_fast32_t result;

    serial_ll_next_timeout = Clock_getTicks() + PRX_TIME_MS * 100;

    while (1) {
        // Just keep listening, unless we have a timeout.
        if (serial_ll_next_timeout && Clock_getTicks() >= serial_ll_next_timeout) {
            serial_timeout();
        }

        // This blocks on a semaphore while waiting to return, so it's safe
        //  not to have a Task_yield() in this.
        result = UART_read(uart, input, 1);

        if (result == 1 && input[0] == SERIAL_PHY_SYNC_WORD) {
            // Got the sync word, now try to read a header:
            result = UART_read(uart, &header_in, sizeof(serial_header_t));
            if (result == sizeof(serial_header_t)
                    && validate_header(&header_in)) {
                if (header_in.payload_len) {
                    result = UART_read(uart, input, header_in.payload_len);
                    if (result == header_in.payload_len) {
                        // RXed good.
                        serial_rx_done(&header_in, input);
                    }
                } else {
                    // RXed good.
                    serial_rx_done(&header_in, input);
                }
            }
        }
    }
}

void serial_init() {
    UART_Params_init(&uart_params);
    uart_params.baudRate = 38400;
    uart_params.readDataMode = UART_DATA_BINARY;
    uart_params.writeDataMode = UART_DATA_BINARY;
    uart_params.readMode = UART_MODE_BLOCKING;
    uart_params.writeMode = UART_MODE_BLOCKING;
    uart_params.readEcho = UART_ECHO_OFF;
    uart_params.readReturnMode = UART_RETURN_FULL;
    uart_params.parityType = UART_PAR_NONE;
    uart_params.stopBits = UART_STOP_ONE;

//    Clock_Params clockParams;
//    Error_Block eb;
//    Error_init(&eb);
//    Clock_Params_init(&clockParams);
//    clockParams.period = PRX_TIME_MS*100;
//    clockParams.startFlag = FALSE;
//    serial_timeout_clock_h = Clock_create(serial_clock_swi, 2, &clockParams, &eb);

    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = serial_task_stack;
    taskParams.stackSize = SERIAL_STACKSIZE;
    taskParams.priority = 1;
    Task_construct(&serial_task, serial_task_fn, &taskParams, NULL);

    // It's not actually possible for a qbadge to be externally powered,
    //  so we're not even going to check.
    serial_pin_h = PIN_open(&serial_pin_state, serial_gpio_prx);

    // This will start up the UART, and configure our GPIO (again).
    serial_enter_prx();

    serial_ll_state = SERIAL_LL_STATE_NC_PRX;
}
