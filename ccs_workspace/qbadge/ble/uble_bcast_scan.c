#include <string.h>
#include <stdlib.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>

#include <ti/grlib/grlib.h>

#include "bcomdef.h"

#include "board.h"

#ifdef USE_RCOSC
#include <ble/rcosc_calibration.h>
#endif // USE_RCOSC

// DriverLib
#include <driverlib/aon_batmon.h>
#include "uble.h"
#include "ugap.h"
#include "urfc.h"

#include <ble/util.h>
#include "gap.h"

#include <ble/uble_bcast_scan.h>

#include <badge.h>
#include <qc16.h>
#include <qc16_serial_common.h>

// Task configuration
#define UBS_TASK_PRIORITY                     3

#ifndef UBS_TASK_STACK_SIZE
#define UBS_TASK_STACK_SIZE                   800
#endif

// RTOS Event to queue application events
#define UEB_QUEUE_EVT                         UTIL_QUEUE_EVENT_ID // Event_Id_30

// Application Events
#define UBS_EVT_MICROBLESTACK   0x0002


/*********************************************************************
 * TYPEDEFS
 */

// App to App event
typedef struct {
    uint16 event;
    uint8 data;
} ubsEvt_t;

/*********************************************************************
 * LOCAL VARIABLES
 */

// Event globally used to post local events and pend on local events.
static Event_Handle syncEvent;

// Queue object used for app messages
static Queue_Struct appMsg;
static Queue_Handle appMsgQueue;

// Task configuration
Task_Struct ubsTask;
uint8 ubsTaskStack[UBS_TASK_STACK_SIZE];

static bool UBLEBcastScan_initObserver(void);

// TODO: Clean up this next part here:
// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
uint8 advertData[30] =
{
 // Flags; this sets the device to use limited discoverable
 // mode (advertises for 30 seconds at a time) instead of general
 // discoverable mode (advertises indefinitely)
 2,   // length of this data
 GAP_ADTYPE_FLAGS, // 0x01
 GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED, // 0x04

 // Appearance: This is a #badgelife header.
 3,    // Length of this data
 GAP_ADTYPE_APPEARANCE, // Data type: "Appearance" // 0x19
 0xDC, // DC
 0x19, // 19 #badgelife

 // complete name
 QC16_BADGE_NAME_LEN+1, // index 7   // length of this data
 GAP_ADTYPE_LOCAL_NAME_COMPLETE, // TODO: is it ok that there is no null term?
 'D',
 'U',
 'P',
 'L',
 'i',
 'C',
 'O',
 ' ',
 ' ',
 // Queercon data: ID, current icon, etc
   11, // length of this data including the data type byte
   GAP_ADTYPE_MANUFACTURER_SPECIFIC, // manufacturer specific adv data type // 0xff
   0xD3, // Company ID - Fixed (queercon)
   0x04, // Company ID - Fixed (queercon)
   0x00, // Badge ID MSB //.22 // TODO: confirm msb/lsb here
   0x00, // Badge ID LSB //.23
   0x00, // TYPE FLAG (BIT7=UBER; BIT6=HANDLER; BIT5-0=ELEMENT)
   0x00, // SPARE
   0x00, // SPARE
   0x00, // SPARE
   0x00, // CHECK // .28
   0x00, // CHECK // .39
};

typedef struct {
    uint16_t badge_id;
    uint16_t spare1;
    uint16_t spare2;
    uint16_t crc16;
} qc16_ble_t;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void UBLEBcastScan_init(void);
static void UBLEBcastScan_taskFxn(UArg a0, UArg a1);

static void UBLEBcastScan_processAppMsg(ubsEvt_t *pMsg);

static void UBLEBcastScan_bcast_stateChangeCB(ugapBcastState_t newState);
static void UBLEBcastScan_bcast_advPrepareCB(void);
static void UBLEBcastScan_bcast_advDoneCB(bStatus_t status);

static void UBLEBcastScan_scan_stateChangeCB(ugapObserverScan_State_t newState);
static void UBLEBcastScan_scan_indicationCB(bStatus_t status, uint8_t len, uint8_t *pPayload);
static void UBLEBcastScan_scan_windowCompleteCB(bStatus_t status);

static bStatus_t UBLEBcastScan_enqueueMsg(uint16 event, uint8 data);

static void UBLEBcastScan_eventProxy(void);

bool UBLEBcastScan_initBcast(void);


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      UBLEBcastScan_createTask
 *
 * @brief   Task creation function for the Micro Eddystone Beacon.
 *
 * @param   None.
 *
 * @return  None.
 */
void UBLEBcastScan_createTask(void)
{
    Task_Params taskParams;

    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = ubsTaskStack;
    taskParams.stackSize = UBS_TASK_STACK_SIZE;
    taskParams.priority = UBS_TASK_PRIORITY;

    Task_construct(&ubsTask, UBLEBcastScan_taskFxn, &taskParams, NULL);

#ifdef USE_RCOSC
    RCOSC_enableCalibration();
#endif // USE_RCOSC
}

/*********************************************************************
 * @fn      UBLEBcastScan_init
 *
 * @brief   Initialization function for the Micro Eddystone Beacon App
 *          Task. This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification ...).
 *
 * @param   none
 *
 * @return  none
 */
static void UBLEBcastScan_init(void)
{

    // Create an RTOS event used to wake up this application to process events.
    syncEvent = Event_create(NULL, NULL);

    // Create an RTOS queue for message from profile to be sent to app.
    appMsgQueue = Util_constructQueue(&appMsg);

    // Default is not to switch antenna
    uble_registerAntSwitchCB(NULL);

    uble_stackInit(UBLE_ADDRTYPE_PUBLIC, NULL, UBLEBcastScan_eventProxy,
                   RF_TIME_CRITICAL);

    UBLEBcastScan_initObserver();
    UBLEBcastScan_initBcast();
}


/*********************************************************************
 * @fn      UBLEBcastScan_processEvent
 *
 * @brief   Application task entry point for the Micro Eddystone Beacon.
 *
 * @param   none
 *
 * @return  none
 */
static void UBLEBcastScan_taskFxn(UArg a0, UArg a1)
{
    volatile uint32 keyHwi;

    // Initialize application
    UBLEBcastScan_init();

    for (;;)
    {
        // Waits for an event to be posted associated with the calling thread.
        // Note that an event associated with a thread is posted when a
        // message is queued to the message receive queue of the thread
        Event_pend(syncEvent, Event_Id_NONE, UEB_QUEUE_EVT, BIOS_WAIT_FOREVER);

        // If RTOS queue is not empty, process app message.
        while (!Queue_empty(appMsgQueue))
        {
            ubsEvt_t *pMsg;

            // malloc() is not thread safe. Must disable HWI.
            keyHwi = Hwi_disable();
            pMsg = (ubsEvt_t *) Util_dequeueMsg(appMsgQueue);
            Hwi_restore(keyHwi);

            if (pMsg)
            {
                // Process message.
                UBLEBcastScan_processAppMsg(pMsg);

                // free() is not thread safe. Must disable HWI.
                keyHwi = Hwi_disable();

                // Free the space from the message.
                free(pMsg);
                Hwi_restore(keyHwi);
            }
        }
    }
}

/*********************************************************************
 * @fn      UBLEBcastScan_processAppMsg
 *
 * @brief   Process an incoming callback from a profile.
 *
 * @param   pMsg - message to process
 *
 * @return  None.
 */
static void UBLEBcastScan_processAppMsg(ubsEvt_t *pMsg)
{
    switch (pMsg->event)
    {
    case UBS_EVT_MICROBLESTACK:
        uble_processMsg();
        break;

    default:
        // Do nothing.
        break;
    }
}

/*********************************************************************
 * @fn      UBLEBcastScan_bcast_stateChange_CB
 *
 * @brief   Callback from Micro Broadcaster indicating a state change.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void UBLEBcastScan_bcast_stateChangeCB(ugapBcastState_t newState)
{
    switch (newState)
    {
    case UGAP_BCAST_STATE_INITIALIZED:
        break;

    case UGAP_BCAST_STATE_IDLE:
        break;

    case UGAP_BCAST_STATE_ADVERTISING:
        break;

    case UGAP_BCAST_STATE_WAITING:
        break;

    default:
        break;
    }
}

/*********************************************************************
 * @fn      UBLEBcastScan_bcast_advPrepareCB
 *
 * @brief   Callback from Micro Broadcaster notifying that the next
 *          advertising event is about to start so it's time to update
 *          the adv payload.
 *
 * @param   None.
 *
 * @return  None.
 */
static void UBLEBcastScan_bcast_advPrepareCB(void) {
    // TODO: Elsewhere this may have problems without a null term, like on the receiving end...
    char *name = (char *) &advertData[9];
    qc16_ble_t *badge_frame = (qc16_ble_t *) &advertData[22];
    memcpy(name, badge_conf.handle, QC16_BADGE_NAME_LEN);
    badge_frame->badge_id = badge_conf.badge_id;
    badge_frame->spare1 = 0;
    badge_frame->spare2 = 0;
    badge_frame->crc16 = crc16_buf((uint8_t *) badge_frame, sizeof(qc16_ble_t)-2);
}

/*********************************************************************
 * @fn      UBLEBcastScan_bcast_advDoneCB
 *
 * @brief   Callback from Micro Broadcaster notifying that an
 *          advertising event has been done.
 *
 * @param   status - How the last event was done. SUCCESS or FAILURE.
 *
 * @return  None.
 */
static void UBLEBcastScan_bcast_advDoneCB(bStatus_t status)
{
}

/*********************************************************************
 * @fn      UBLEBcastScan_enqueueMsg
 *
 * @brief   Creates a message and puts the message in RTOS queue.
 *
 * @param   event - message event.
 * @param   data - message data.
 *
 * @return  TRUE or FALSE
 */
static bStatus_t UBLEBcastScan_enqueueMsg(uint16 event, uint8 data)
{
    volatile uint32 keyHwi;
    ubsEvt_t *pMsg;
    uint8_t status = FALSE;

    // malloc() is not thread safe. Must disable HWI.
    keyHwi = Hwi_disable();

    // Create dynamic pointer to message.
    pMsg = (ubsEvt_t*) malloc(sizeof(ubsEvt_t));
    if (pMsg != NULL)
    {
        pMsg->event = event;
        pMsg->data = data;

        // Enqueue the message.
        status = Util_enqueueMsg(appMsgQueue, syncEvent, (uint8*) pMsg);
    }
    Hwi_restore(keyHwi);
    return status;
}

/*********************************************************************
 * @fn      UBLEBcastScan_eventProxy
 *
 * @brief   Proxy function for posting an event from the uBLE Stack
 *          to the application
 *
 * @return  None
 */
void UBLEBcastScan_eventProxy(void)
{
    UBLEBcastScan_enqueueMsg(UBS_EVT_MICROBLESTACK, 0);
}

/*********************************************************************
 * @fn      UBLEBcastScan_initBcast
 *
 * @brief   Initialize broadcaster functionality
 *
 * @return  status - true if successful false otherwise
 */
bool UBLEBcastScan_initBcast(void)
{
    ugapBcastCBs_t bcastCBs = {
        UBLEBcastScan_bcast_stateChangeCB,
        UBLEBcastScan_bcast_advPrepareCB,
        UBLEBcastScan_bcast_advDoneCB
    };

    /* Initilaize Micro GAP Broadcaster Role */
    if (SUCCESS == ugap_bcastInit(&bcastCBs))
    {
        uble_setParameter(UBLE_PARAM_ADVDATA, sizeof(advertData), advertData);
        return true;
    }

    return false;
}

/*********************************************************************
 * @fn      UBLEBcastScan_initObserver
 *
 * @brief   Initialze observer functionality
 *
 * @return  status - true if successful false otherwise
 */
bool UBLEBcastScan_initObserver(void)
{
    ugapObserverScanCBs_t observerCBs = {
        UBLEBcastScan_scan_stateChangeCB,
        UBLEBcastScan_scan_indicationCB,
        UBLEBcastScan_scan_windowCompleteCB
    };

    /* Initialize Micro GAP Observer Role */
    if (SUCCESS == ugap_scanInit(&observerCBs))
    {
        ugap_scanRequest(UBLE_ADV_CHAN_ALL, 160, 320);
        return true;
    }

    return false;
}

/*********************************************************************
 * @fn      UBLEBcastScan_scan_stateChangeCB
 *
 * @brief   Callback from Micro Observer indicating a state change.
 *
 * @param   newState - new state
 *
 * @return  None.
 */
static void UBLEBcastScan_scan_stateChangeCB(ugapObserverScan_State_t newState)
{
    switch (newState)
    {
    case UGAP_SCAN_STATE_INITIALIZED:
        break;

    case UGAP_SCAN_STATE_IDLE:
        break;

    case UGAP_SCAN_STATE_SCANNING:
        break;

    case UGAP_SCAN_STATE_WAITING:
        break;

    case UGAP_SCAN_STATE_SUSPENDED:
        break;

    default:
        break;
    }
}

/*********************************************************************
 * @fn      UBLEBcastScan_scan_indicationCB
 *
 * @brief   Callback from Micro observer notifying that a advertising
 *          packet is received.
 *
 * @param   status status of a scan
 * @param   len length of the payload
 * @param   pPayload pointer to payload
 *
 * @return  None.
 */
static void UBLEBcastScan_scan_indicationCB(bStatus_t status, uint8_t len,
                                            uint8_t *pPayload)
{
    volatile static uint8  chan;
    volatile static uint32 timeStamp;

    /* We have an advertisment packet:
     *
     * | Preamble  | Access Addr | PDU         | CRC     |
     * | 1-2 bytes | 4 bytes     | 2-257 bytes | 3 bytes |
     *
     * The PDU is expended to:
     * | Header  | Payload     |
     * | 2 bytes | 1-255 bytes |
     *
     * The Header is expended to:
     * | PDU Type...      | Length |
     * | 1 byte           | 1 byte |
     *
     * PDU types are:
     * ADV_IND (0x00): Connectable undirected advertising which can be connected to by any BLE central.
     * ADV_DIRECT_IND (0x01): Connectable directed advertising which can be connected to by one specific Central.
     * ADV_NONCONN_IND (0x02): Non-connectable undirected advertising which cannot be connected to and cannot respond to a scan request.
     * SCAN_REQ (0x03): sent by the Central requesting a scan response (which is a way to send more data via advertising but the response is directed specifically to the requester).
     * SCAN_RSP (0x04): The scan response packet containing any additional info sent by the peripheral.
     * CONNECT_REQ (0x05): a connection request packet sent by the Central to connect to a specific peripheral.
     * ADV_SCAN_IND (0x06): Scannable undirected advertising which cannot be connected to but which can respond to a scan request.
     *
     * The Payload is expended to:
     * | AdvA    | AdvData    |
     * | 6 bytes | 0-31 bytes |
     *
     * The radio stripps the CRC and replaces it with the postfix.
     *
     * The Postfix is expended to:
     * | RSSI   | Status | TimeStamp |
     * | 1 byte | 1 byte | 4 bytes   |
     *
     * The Status is expended to:
     * | bCrcErr | bIgnore | channel  |
     * | bit 7   | bit 6   | bit 5..0 |
     *
     * Note the advPke is the beginning of PDU; the dataLen includes
     * the postfix length.
     *
     */

    // What we receive here is:
    /*
     * | Type |      |
     * | RxAdd|Len(1)|AdvAddr|AdvData   | RSSI   | Status | TimeStamp |
     * |1 byte|1 byte|6 bytes|0-31 bytes| 1 byte | 1 byte | 4 bytes   |
     *
     * The PDU is expended to:
     * | Header  | Payload     |
     * | 2 bytes | 1-255 bytes |
     *
     * The Header is expended to:
     * | PDU Type...RxAdd | Length |
     * | 1 byte           | 1 byte |
     *
     * The Payload is expended to:
     * | AdvA    | AdvData    |
     * | 6 bytes | 0-31 bytes |
     *
     * The Postfix is expended to:
     * | RSSI   | Status | TimeStamp |
     * | 1 byte | 1 byte | 4 bytes   |
     *
     * The Status is expended to:
     * | bCrcErr | bIgnore | channel  |
     * | bit 7   | bit 6   | bit 5..0 |
     */
    if (status == SUCCESS)
    {
        // We want: pPayload[0] == 0x02; (non-connectable non-scannable)
        // TODO: What do DC Furs badges look like?
        // pPayload[1] == length
        // Now, look for:
        //  1. GAP_ADTYPE_LOCAL_NAME_COMPLETE
        //    followed by a handle
        //  2. GAP_ADTYPE_MANUFACTURER_SPECIFIC
        //     0xD3
        //     0x04
        //    followed by our beacon struct, whatever that is.
        uint8_t rssi = *(uint8_t *)(pPayload + len - 6);
        char *name;
        qc16_ble_t *badge_frame;
        // the MAC address is 6 bytes at pPayload[2]
        // the data begins at pPayload[8]
        uint8_t *advData = &pPayload[8];
        // The advData is up to 31 bytes long, which should be len-8-6
        uint8_t i = 0;
        uint8_t seems_queercon = 0;
        while (i < len-8-6) {
            uint8_t section_len = advData[i];
            switch(advData[i+1]) {
            case GAP_ADTYPE_LOCAL_NAME_COMPLETE:
                name = (char *) &advData[i+2];
                seems_queercon |= 0xf0;
                break;
            case GAP_ADTYPE_MANUFACTURER_SPECIFIC:
                if (advData[i+2] == 0xD3 && advData[i+3] == 0x04) {
                    // this is the queercon ID.
                    badge_frame = (qc16_ble_t *) &advData[i+4];
                    seems_queercon |= 0x0f;
                }
                break;
            }
            i += section_len+1;
        }
        if (i >= len-8-6) {
            // TODO
        }

        if (seems_queercon == 0xFF) {
            // TODO: validate CRC
            // this looks like a badge.
            set_badge_seen(badge_frame->badge_id, name);
        }

        // TODO: clean up:
//        devAddr = Util_convertBdAddr2Str(pPayload + 2);
//        chan = (*(pPayload + len - 5) & 0x3F);
//        timeStamp = *(uint32 *)(pPayload + len - 4);
        // We got a message!
    }
    else
    {
        /* Rx buffer is full */
    }
}

/*********************************************************************
 * @fn      UBLEBcastScan_scan_windowCompleteCB
 *
 * @brief   Callback from Micro Broadcaster notifying that a
 *          scan window is completed.
 *
 * @param   status - How the last event was done. SUCCESS or FAILURE.
 *
 * @return  None.
 */
static void UBLEBcastScan_scan_windowCompleteCB (bStatus_t status)
{
    ugap_bcastStart(1);
}

/*********************************************************************
 *********************************************************************/
