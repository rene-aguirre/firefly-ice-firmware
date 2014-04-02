/* 
* Copyright (c) 2013, Nordic Semiconductor ASA
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
* 
* - Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
* 
* - Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
* 
* - The name of Nordic Semiconductor ASA may not be used to endorse or promote
*   products derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
* This file is autogenerated by nRFgo Studio .3031 
*/

#ifndef SETUP_MESSAGES_H__
#define SETUP_MESSAGES_H__

#if 0
#include "hal_platform.h" 
#include "aci.h"
#endif

#define SETUP_ID 0
#define SETUP_FORMAT 3 /** nRF8001 D */
#define ACI_DYNAMIC_DATA_SIZE 158

/* Service: Gap - Characteristic: Device name - Pipe: SET */
#define PIPE_GAP_DEVICE_NAME_SET          1
#define PIPE_GAP_DEVICE_NAME_SET_MAX_SIZE 20

/* Service: Device Information - Characteristic: Model Number String - Pipe: TX */
#define PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX          2
#define PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX_MAX_SIZE 20

/* Service: Device Information - Characteristic: Model Number String - Pipe: RX */
#define PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_RX          3
#define PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_RX_MAX_SIZE 20


#define NUMBER_OF_PIPES 3

#define SERVICES_PIPE_TYPE_MAPPING_CONTENT {\
  {ACI_STORE_LOCAL, ACI_SET},   \
  {ACI_STORE_LOCAL, ACI_TX},   \
  {ACI_STORE_LOCAL, ACI_RX},   \
}

#define GAP_PPCP_MAX_CONN_INT 0xffff /**< Maximum connection interval as a multiple of 1.25 msec , 0xFFFF means no specific value requested */
#define GAP_PPCP_MIN_CONN_INT  0xffff /**< Minimum connection interval as a multiple of 1.25 msec , 0xFFFF means no specific value requested */
#define GAP_PPCP_SLAVE_LATENCY 0
#define GAP_PPCP_CONN_TIMEOUT 0xffff /** Connection Supervision timeout multiplier as a multiple of 10msec, 0xFFFF means no specific value requested */

/** @brief do a set_local_data for PIPE_GAP_DEVICE_NAME_SET
 *  @param src source buffer to send data from
 *  @param size the number of bytes to send. Maximum size is 20
 *  @details use this function to do a set_local_data for PIPE_GAP_DEVICE_NAME_SET. If no transaction are currently
 *  running, the set will be immediate, otherwise, it will be done at the end of the current transaction
 *  when services_update_pipes will be called.
 */
void services_set_gap_device_name(void *src, int size);

/** @brief send a new value for PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX
 *  @param src source buffer to send data from.
 *             Presentation format: UTF-8 string
 *  @param size the number of bytes to send. Maximum size is 20
 *  @param is_freshest_sample set it to true if you want to overwrite an eventual pending transaction on this pipe.
 *  @details use this function to send a new value for PIPE_DEVICE_INFORMATION_MODEL_NUMBER_STRING_TX. If no transaction are currently
 *  running, the send will be immediate, otherwise, it will be done at the end of the current transaction
 *  when services_update_pipes will be called. If a transaction on this pipe is already pending, then this function
 *  will not overwrite the data of the previous transaction and return false.
 *  @return : true if is_freshest_sample true, otherwise return false if a transaction on this pipe is already pending, true otherwise.
 */
bool services_send_device_information_model_number_string(void *src, int size, bool is_freshest_sample);

/** @brief function to trig pending transaction on pipes
 *  @details This function check for each pipe if it has a pending transaction (send/rx_request/ack)
 *   and if so executes this transaction.
 *   This function should be called in the APP_RUN state of the process function of the application.
 */
void services_update_pipes(void);

#define NB_SETUP_MESSAGES 16
#define SETUP_MESSAGES_CONTENT {\
    {0x00,\
        {\
            0x07,0x06,0x00,0x00,0x03,0x02,0x41,0xfe,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x03,0x01,0x01,0x00,0x00,0x06,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x03,0x90,0x00,0xff,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x38,0xff,0xff,0x02,0x58,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x05,0x06,0x10,0x54,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x00,0x04,0x04,0x02,0x02,0x00,0x01,0x28,0x00,0x01,0x00,0x18,0x04,0x04,0x05,0x05,0x00,\
            0x02,0x28,0x03,0x01,0x0e,0x03,0x00,0x00,0x2a,0x04,0x14,0x14,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x1c,0x14,0x00,0x03,0x2a,0x00,0x01,0x68,0x77,0x69,0x64,0x31,0x31,0x32,0x32,0x33,0x33,\
            0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,0x38,0x38,0x04,0x04,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x38,0x05,0x05,0x00,0x04,0x28,0x03,0x01,0x02,0x05,0x00,0x01,0x2a,0x06,0x04,0x03,0x02,\
            0x00,0x05,0x2a,0x01,0x01,0x00,0x00,0x04,0x04,0x05,0x05,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x54,0x06,0x28,0x03,0x01,0x02,0x07,0x00,0x04,0x2a,0x06,0x04,0x09,0x08,0x00,0x07,0x2a,\
            0x04,0x01,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0x04,0x04,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x70,0x02,0x02,0x00,0x08,0x28,0x00,0x01,0x01,0x18,0x04,0x04,0x02,0x02,0x00,0x09,0x28,\
            0x00,0x01,0x0a,0x18,0x04,0x04,0x05,0x05,0x00,0x0a,0x28,0x03,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x8c,0x01,0x16,0x0b,0x00,0x24,0x2a,0x56,0x14,0x15,0x14,0x00,0x0b,0x2a,0x24,0x01,0x6e,\
            0x52,0x46,0x78,0x78,0x78,0x78,0x78,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xa8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x04,0x08,0x07,0x00,0x0c,0x29,0x04,0x01,\
            0x19,0x00,0x00,0x00,0x01,0x00,0x00,0x46,0x14,0x03,0x02,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x0a,0x06,0x20,0xc4,0x0d,0x29,0x02,0x01,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x17,0x06,0x40,0x00,0x2a,0x00,0x01,0x00,0x80,0x04,0x00,0x03,0x00,0x00,0x2a,0x24,0x01,0x00,0x0a,0x04,\
            0x00,0x0b,0x00,0x0d,\
        },\
    },\
    {0x00,\
        {\
            0x09,0x06,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x06,0x06,0xf0,0x00,0x03,0x14,0xbd,\
        },\
    },\
}

#endif
