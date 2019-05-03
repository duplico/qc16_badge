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

// DriverLib
#include <driverlib/aon_batmon.h>
#include "uble.h"
#include "ugap.h"
#include "urfc.h"

#include "util.h"
#include "gap.h"

#include "uble_bcast_scan.h"

// Eddystone Base 128-bit UUID: EE0CXXXX-8786-40BA-AB96-99B91AC981D8
#define EDDYSTONE_BASE_UUID_128( uuid )  0xD8, 0x81, 0xC9, 0x1A, 0xB9, 0x99, \
                                         0x96, 0xAB, 0xBA, 0x40, 0x86, 0x87, \
                           LO_UINT16( uuid ), HI_UINT16( uuid ), 0x0C, 0xEE
// Task configuration
#define UBS_TASK_PRIORITY                     3

#ifndef UBS_TASK_STACK_SIZE
#define UBS_TASK_STACK_SIZE                   800
#endif

// TODO: This is dumb:
// RTOS Event to queue application events
#define UEB_QUEUE_EVT                         UTIL_QUEUE_EVENT_ID // Event_Id_30

// Application Events
#define UBS_EVT_MICROBLESTACK   0x0002

// Eddystone definitions
#define EDDYSTONE_SERVICE_UUID                  0xFEAA

#define EDDYSTONE_FRAME_TYPE_UID                0x00
#define EDDYSTONE_FRAME_TYPE_URL                0x10
#define EDDYSTONE_FRAME_TYPE_TLM                0x20

#define EDDYSTONE_FRAME_OVERHEAD_LEN            8
#define EDDYSTONE_SVC_DATA_OVERHEAD_LEN         3
#define EDDYSTONE_MAX_URL_LEN                   18

// # of URL Scheme Prefix types
#define EDDYSTONE_URL_PREFIX_MAX        4
// # of encodable URL words
#define EDDYSTONE_URL_ENCODING_MAX      14

#define EDDYSTONE_URI_DATA_DEFAULT      "http://www.ti.com/ble"

/*********************************************************************
 * TYPEDEFS
 */

// App to App event
typedef struct {
  uint16 event;
  uint8 data;
} ubsEvt_t;

// Eddystone UID frame
typedef struct {
  uint8 frameType;      // UID
  int8 rangingData;
  uint8 namespaceID[10];
  uint8 instanceID[6];
  uint8 reserved[2];
} eddystoneUID_t;

// Eddystone URL frame
typedef struct {
  uint8 frameType;      // URL | Flags
  int8 txPower;
  uint8 encodedURL[EDDYSTONE_MAX_URL_LEN];  // the 1st byte is prefix
} eddystoneURL_t;

// Eddystone TLM frame
typedef struct {
  uint8 frameType;      // TLM
  uint8 version;        // 0x00 for now
  uint8 vBatt[2];       // Battery Voltage, 1mV/bit, Big Endian
  uint8 temp[2];        // Temperature. Signed 8.8 fixed point
  uint8 advCnt[4];      // Adv count since power-up/reboot
  uint8 secCnt[4];      // Time since power-up/reboot
                          // in 0.1 second resolution
} eddystoneTLM_t;

typedef union {
  eddystoneUID_t uid;
  eddystoneURL_t url;
  eddystoneTLM_t tlm;
} eddystoneFrame_t;

typedef struct {
  uint8 length1;        // 2
  uint8 dataType1;      // for Flags data type (0x01)
  uint8 data1;          // for Flags data (0x04)
  uint8 length2;        // 3
  uint8 dataType2;      // for 16-bit Svc UUID list data type (0x03)
  uint8 data2;          // for Eddystone UUID LSB (0xAA)
  uint8 data3;          // for Eddystone UUID MSB (0xFE)
  uint8 length;         // Eddystone service data length
  uint8 dataType3;      // for Svc Data data type (0x16)
  uint8 data4;          // for Eddystone UUID LSB (0xAA)
  uint8 data5;          // for Eddystone UUID MSB (0xFE)
  eddystoneFrame_t frame;
} eddystoneAdvData_t;

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

#if defined(FEATURE_OBSERVER)
static bool UBLEBcastScan_initObserver(void);
#endif /* FEATURE_OBSERVER */


// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
uint8 advertData[23] =
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
    15,   // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'Q',
    'C',
    'u',
    'b',
    'e',
    ' ',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
};

// Array of URL Scheme Prefices
static char* eddystoneURLPrefix[EDDYSTONE_URL_PREFIX_MAX] = { "http://www.",
    "https://www.", "http://", "https://" };

// Array of URLs to be encoded
static char* eddystoneURLEncoding[EDDYSTONE_URL_ENCODING_MAX] = { ".com/",
    ".org/", ".edu/", ".net/", ".info/", ".biz/", ".gov/", ".com/", ".org/",
    ".edu/", ".net/", ".info/", ".biz/", ".gov/" };

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void UBLEBcastScan_init(void);
static void UBLEBcastScan_taskFxn(UArg a0, UArg a1);
static void UBLEBcastScan_initUID(void);
static void UBLEBcastScan_initURL(void);

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
  ugapBcastCBs_t bcastCBs = {
  UBLEBcastScan_bcast_stateChangeCB,
  UBLEBcastScan_bcast_advPrepareCB,
  UBLEBcastScan_bcast_advDoneCB };

  bStatus_t status;

  // Create an RTOS event used to wake up this application to process events.
  syncEvent = Event_create(NULL, NULL);

  // Create an RTOS queue for message from profile to be sent to app.
  appMsgQueue = Util_constructQueue(&appMsg);

  // Default is not to switch antenna
  uble_registerAntSwitchCB(NULL);

  uble_stackInit(UBLE_ADDRTYPE_PUBLIC, NULL, UBLEBcastScan_eventProxy,
                 RF_TIME_CRITICAL);
  ugap_bcastInit(&bcastCBs);

  status = uble_stackInit(UBLE_ADDRTYPE_PUBLIC, NULL, UBLEBcastScan_eventProxy,
                          RF_TIME_CRITICAL);

  UBLEBcastScan_initObserver();
  UBLEBcastScan_initBcast();
}

/*********************************************************************
 * @fn      UBLEBcastScan_bcast_updateAdvData(void)
 *
 * @brief   Update Adv Data.
 *
 * @param   None.
 *
 * @return  None.
 */
static void UBLEBcastScan_bcast_updateAdvData(void)
{
    // TODO: Update data.
//    uble_setParameter(UBLE_PARAM_ADVDATA,
//                          EDDYSTONE_FRAME_OVERHEAD_LEN + eddystoneAdv.length,
//                          &eddystoneAdv);
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
      keyHwi = Hwi_disable(); // TODO: Figure out what's going on here.
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
static void UBLEBcastScan_bcast_advPrepareCB(void)
{
  UBLEBcastScan_bcast_updateAdvData();
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
    UBLEBcastScan_bcast_advDoneCB };

  /* Initilaize Micro GAP Broadcaster Role */
  if (SUCCESS == ugap_bcastInit(&bcastCBs))
  {
    // TODO: Initialize the advertisement:
//    uble_setParameter(UBLE_PARAM_ADVDATA,
//                          EDDYSTONE_FRAME_OVERHEAD_LEN + eddystoneAdv.length,
//                          &eddystoneAdv);
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
    UBLEBcastScan_scan_windowCompleteCB };

  /* Initialize Micro GAP Observer Role */
  if (SUCCESS == ugap_scanInit(&observerCBs))
  {

    /* This is the spot to do scan request without using the keypress control */
    ugap_scanRequest(UBLE_ADV_CHAN_ALL, 160, 320);

    return true;
  }

  return false;
}

// TODO: Use this to start broadcasting:
// ugap_bcastStart(numAdv)
//  and to stop:
// ugap_bcastStop()

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
  volatile static char  *devAddr;
  volatile static uint8  chan;
  volatile static uint32 timeStamp;

  /* We have an advertisment packe:
   *
   * | Preamble  | Access Addr | PDU         | CRC     |
   * | 1-2 bytes | 4 bytes     | 2-257 bytes | 3 bytes |
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
  if (status == SUCCESS)
  {
    devAddr = Util_convertBdAddr2Str(pPayload + 2);
    chan = (*(pPayload + len - 5) & 0x3F);
    timeStamp = *(uint32 *)(pPayload + len - 4);
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

}

/*********************************************************************
 *********************************************************************/
