/******************************************************************************

 @file  util.c

 @brief This file contains utility functions commonly used by
        BLE applications for CC26xx with TIRTOS.

 Group: WCS, BTS
 Target Device: cc2640r2

 ******************************************************************************
 
 Copyright (c) 2014-2019, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/hal/Hwi.h>

#ifdef USE_ICALL
#include <icall.h>
#else
#include <stdlib.h>
#endif

#include "bcomdef.h"
#include "util.h"

// RTOS queue for profile/app messages.
typedef struct _queueRec_
{
  Queue_Elem _elem;          // queue element
  uint8_t *pData;            // pointer to app data
} queueRec_t;

/*********************************************************************
 * @fn      Util_constructQueue
 *
 * @brief   Initialize an RTOS queue to hold messages to be processed.
 *
 * @param   pQueue - pointer to queue instance structure.
 *
 * @return  A queue handle.
 */
Queue_Handle Util_constructQueue(Queue_Struct *pQueue)
{
  // Construct a Queue instance.
  Queue_construct(pQueue, NULL);

  return Queue_handle(pQueue);
}

/*********************************************************************
 * @fn      Util_enqueueMsg
 *
 * @brief   Creates a queue node and puts the node in RTOS queue.
 *
 * @param   msgQueue - queue handle.
 * @param   event - thread's event processing handle that queue is
 *                associated with.
 * @param   pMsg - pointer to message to be queued
 *
 * @return  TRUE if message was queued, FALSE otherwise.
 */
uint8_t Util_enqueueMsg(Queue_Handle msgQueue,
                        Event_Handle event,
                        uint8_t *pMsg)
{
  queueRec_t *pRec;

  // Allocated space for queue node.
#ifdef USE_ICALL
  if ((pRec = ICall_malloc(sizeof(queueRec_t))))
#else
  if ((pRec = (queueRec_t *)malloc(sizeof(queueRec_t))))
#endif
  {
    pRec->pData = pMsg;

    // This is an atomic operation
    Queue_put(msgQueue, &pRec->_elem);

    // Wake up the application thread event handler.
    if (event)
    {
      Event_post(event, UTIL_QUEUE_EVENT_ID);
    }

    return TRUE;
  }

  // Free the message.
#ifdef USE_ICALL
  ICall_free(pMsg);
#else
  free(pMsg);
#endif

  return FALSE;
}

/*********************************************************************
 * @fn      Util_dequeueMsg
 *
 * @brief   Dequeues the message from the RTOS queue.
 *
 * @param   msgQueue - queue handle.
 *
 * @return  pointer to dequeued message, NULL otherwise.
 */
uint8_t *Util_dequeueMsg(Queue_Handle msgQueue)
{
  queueRec_t *pRec = Queue_get(msgQueue);

  if (pRec != (queueRec_t *)msgQueue)
  {
    uint8_t *pData = pRec->pData;

    // Free the queue node
    // Note:  this does not free space allocated by data within the node.
#ifdef USE_ICALL
    ICall_free(pRec);
#else
    free(pRec);
#endif

    return pData;
  }

  return NULL;
}

/*********************************************************************
 * @fn      Util_convertBdAddr2Str
 *
 * @brief   Convert Bluetooth address to string. Only needed when
 *          LCD display is used.
 *
 * @param   pAddr - BD address
 *
 * @return  BD address as a string
 */
char *Util_convertBdAddr2Str(uint8_t *pAddr)
{
  uint8_t     charCnt;
  char        hex[] = "0123456789ABCDEF";
  static char str[(2*B_ADDR_LEN)+3];
  char        *pStr = str;

  *pStr++ = '0';
  *pStr++ = 'x';

  // Start from end of addr
  pAddr += B_ADDR_LEN;

  for (charCnt = B_ADDR_LEN; charCnt > 0; charCnt--)
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }
  pStr = NULL;

  return str;
}