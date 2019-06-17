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
#include <ti/drivers/PIN.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>

#include "board.h"
#include <qc16_serial_common.h>
#include <queercon_drivers/qbadge_serial.h>

#define SERIAL_STACKSIZE 1024
Task_Struct serial_task;
char serial_task_stack[SERIAL_STACKSIZE];
UART_Handle uart;
UART_Params uart_params;

PIN_Handle serial_pin_h;
PIN_State serial_pin_state;

uint8_t serial_mode;
uint32_t serial_next_timeout;
Clock_Handle serial_timeout_clock_h;

const PIN_Config serial_gpio_startup[] = {
    QC16_PIN_SERIAL_DIO1_ABS | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_SERIAL_DIO2_RTR | PIN_INPUT_EN | PIN_PULLDOWN,
    PIN_TERMINATE
};

const PIN_Config serial_gpio_active[] = {
    QC16_PIN_SERIAL_DIO1_ABS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_SERIAL_DIO2_RTR | PIN_INPUT_EN | PIN_PULLDOWN,
    PIN_TERMINATE
};

const PIN_Config serial_gpio_prx_connected[] = {
    QC16_PIN_SERIAL_DIO1_ABS | PIN_INPUT_EN | PIN_PULLDOWN,
    QC16_PIN_SERIAL_DIO2_RTR | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    PIN_TERMINATE
};

const PIN_Config serial_gpio_ptx_connected[] = {
    QC16_PIN_SERIAL_DIO1_ABS | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH,
    QC16_PIN_SERIAL_DIO2_RTR | PIN_INPUT_EN | PIN_PULLDOWN,
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

void serial_send_ack(UART_Handle uart) {
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

void serial_rx_done(serial_header_t *header, uint8_t *payload) {
    // If this is called, it's already been validated.
    switch(serial_mode) {
    case SERIAL_LL_STATE_NC_PRX:
        // We are expecting a HELO.
        if (header->opcode == SERIAL_OPCODE_HELO) {
            // Set DIO2 high; set DIO1(ABS) to input w/ pulldown
            PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx_connected[0]);
            PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_prx_connected[1]);
            // Send an ACK, set connected.
            serial_send_ack(uart);
            serial_mode = SERIAL_LL_STATE_C_IDLE;
            // Cancel next timeout.
            serial_next_timeout = 0;
        }
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // We are expecting an ACK.
        if (header->opcode == SERIAL_OPCODE_ACK) {
            // DIO1(ABS) output high:
            // DIO2 input with pull-down:
            PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx_connected[0]);
            PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_ptx_connected[1]);
            serial_mode = SERIAL_LL_STATE_C_IDLE;
            // Cancel next timeout.
            serial_next_timeout = 0;
        }
        break;
    case SERIAL_LL_STATE_C_IDLE:
        break;
//    default:
    }
}

void serial_timeout() {
    switch(serial_mode) {
    case SERIAL_LL_STATE_NC_PRX:
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_mode = SERIAL_LL_STATE_NC_PTX;
        uart_params.readTimeout = PTX_TIME_MS * 100;
        serial_next_timeout = Clock_getTicks() + (PTX_TIME_MS * 100);
        uart = UART_open(QC16_UART_PTX, &uart_params);
        // Also, since this is now the TX mode, we need to send a HELO.
        serial_send_helo(uart);
        break;
    case SERIAL_LL_STATE_NC_PTX:
        // Switch UART TX/RX and change timeout.
        UART_close(uart);
        serial_mode = SERIAL_LL_STATE_NC_PRX;
        uart_params.readTimeout = PRX_TIME_MS * 100;
        serial_next_timeout = Clock_getTicks() + (PRX_TIME_MS * 100);
        uart = UART_open(QC16_UART_PRX, &uart_params);
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
    int_fast32_t result;

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

        // Just keep listening, unless we have a timeout.
        if (serial_next_timeout && Clock_getTicks() >= serial_next_timeout) {
            serial_timeout();
        }

        Task_yield();
    }
}

void serial_init() {
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

    serial_mode = SERIAL_LL_STATE_NC_PRX;
    serial_pin_h = PIN_open(&serial_pin_state, serial_gpio_startup);

    if (PIN_getInputValue(QC16_PIN_SERIAL_DIO1_ABS)) {
        // This is not actually possible without modifying the hardware.
    } else {
        // If it's low, we're under our own power.
        PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_active[0]);
        PIN_setConfig(serial_pin_h, PIN_BM_ALL, serial_gpio_active[1]);
    }
}
