/*
 * qbadge_serial.c
 *
 *  Created on: Jun 12, 2019
 *      Author: george
 */
#include <string.h>

#include <xdc/runtime/Error.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include "board.h"
#include "queercon/qbadge_serial.h"

#include <qc16_serial_common.h>

#define SERIAL_STACKSIZE 1024
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];
UART_Handle uart;
UART_Params uart_params;

uint8_t serial_mode;
uint32_t serial_next_timeout;
Clock_Handle serial_timeout_clock_h;

void serial_send_helo(UART_Handle uart) {
    serial_header_t header_out;
    header_out.from_id = 1;
    header_out.opcode = SERIAL_OPCODE_HELO;
    header_out.payload_len = 0;
    header_out.to_id = SERIAL_ID_ANY;
    header_out.crc16_payload = 0xabcd; // TODO
    header_out.crc16_header = 0xabcd; // TODO
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(uart, &syncword, 1);
    UART_write(uart, (uint8_t *)(&header_out), sizeof(serial_header_t));
}

void serial_send_ack(UART_Handle uart) {
    serial_header_t header_out;
    header_out.from_id = 1;
    header_out.opcode = SERIAL_OPCODE_ACK;
    header_out.payload_len = 0;
    header_out.to_id = SERIAL_ID_ANY;
    header_out.crc16_payload = 0xabcd; // TODO
    header_out.crc16_header = 0xabcd; // TODO
    uint8_t syncword = SERIAL_PHY_SYNC_WORD;

    UART_write(uart, &syncword, 1);
    UART_write(uart, (uint8_t *)(&header_out), sizeof(serial_header_t));
}


void serial_clock_swi(UArg a0) {

}

uint8_t validate_header(serial_header_t *header) {
    return 1; // TODO
}

void serial_rx_done(serial_header_t *header, uint8_t *payload) {
    // If this is called, it's already been validated.
    switch(serial_mode) {
    case SERIAL_MODE_NC_PRX:
        // We are expecting a HELO.
        if (header->opcode == SERIAL_OPCODE_HELO) {
            // TODO: set DIO2 high; set DIO1(ABS) to input w/ pulldown
            // Send an ACK, set connected.
            serial_send_ack(uart);
            serial_mode = SERIAL_MODE_C_IDLE;
            // Cancel next timeout.
            serial_next_timeout = 0;
            // TODO: set up DIO.
        }
        break;
    case SERIAL_MODE_NC_PTX:
        // We are expecting an ACK.
        if (header->opcode == SERIAL_OPCODE_ACK) {
            // TODO: set DIO1(ABS) high; set DIO2 to input w/ pulldown
            serial_mode = SERIAL_MODE_C_IDLE;
            // Cancel next timeout.
            serial_next_timeout = 0;
            // TODO: set up DIO.
        }
        break;
    case SERIAL_MODE_C_IDLE:
        // TODO: parse & signal
        break;
//    default:
        // TODO
    }
}

void serial_timeout() {
    switch(serial_mode) {
    case SERIAL_MODE_NC_PRX:
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_mode = SERIAL_MODE_NC_PTX;
        uart_params.readTimeout = PTX_TIME_MS * 100;
        serial_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
        uart = UART_open(QC16_UART_PTX, &uart_params);
        // Also, since this is now the TX mode, we need to send a HELO.
        serial_send_helo(uart);
        break;
    case SERIAL_MODE_NC_PTX:
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_mode = SERIAL_MODE_NC_PRX;
        uart_params.readTimeout = PRX_TIME_MS * 100;
        serial_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
        uart = UART_open(QC16_UART_PRX, &uart_params);
        break;
//    default:
        // TODO
    }
}

void serial_task_fn(UArg a0, UArg a1) {
    // There are two serial modes:
    //  Primary RX - in which we listen for a HELO message, and
    //  Primary TX - in which we send a HELO and wait, very briefly, for ACK.
    volatile serial_header_t header_in;
    volatile uint8_t input[32];
    volatile int_fast32_t result;

    // TODO: This will overflow every ~5 days:
    serial_next_timeout = Clock_getTicks() + PRX_TIME_MS * 100;

    while (1) {
        result = UART_read(uart, input, 1);
        if (result == 1 && input[0] == SERIAL_PHY_SYNC_WORD) {
            // Got the sync word, now try to read a header:
            result = UART_read(uart, &header_in, sizeof(serial_header_t));
            if (result == sizeof(serial_header_t)
                    && validate_header(&header_in)) {
                if (header_in.payload_len) {
                    result = UART_read(uart, input, header_in.payload_len);
                    // TODO: validate len will fit.
                    if (result == header_in.payload_len) {
                        // TODO: validate the crc
                        // RXed good.
                        serial_rx_done(&header_in, input);
                    }
                } else {
                    // RXed good.
                    serial_rx_done(&header_in, input);
                }
            }
        }

        // Just keep listening, unless we have a timeout.
        // TODO: This will overflow every ~5 days: (lol @ battery lasting)
        if (serial_next_timeout && Clock_getTicks() >= serial_next_timeout) {
            serial_timeout();
        }

        Task_yield();
    }
}

void serial_init() {
    // TODO: Read and set ABS

    UART_Params_init(&uart_params);
    uart_params.baudRate = 9600;
    uart_params.readDataMode = UART_DATA_BINARY;
    uart_params.writeDataMode = UART_DATA_BINARY;
    uart_params.readMode = UART_MODE_BLOCKING;
    uart_params.writeMode = UART_MODE_BLOCKING;
    uart_params.readEcho = UART_ECHO_OFF;
    uart_params.readReturnMode = UART_RETURN_FULL;
    uart_params.parityType = UART_PAR_EVEN;
    uart_params.stopBits = UART_STOP_TWO;

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

    uart_params.readTimeout = PRX_TIME_MS * 100;
    uart = UART_open(QC16_UART_PRX, &uart_params);

    serial_mode = SERIAL_MODE_NC_PRX;
}
