/**************************************************************************/
/*!
    @file     msc_uf2.c
    @author   hathach (tinyusb.org)

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2018, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "msc_uf2.h"

#if CFG_TUD_MSC

#include "pstorage.h"

/*------------------------------------------------------------------*/
/* MACRO TYPEDEF CONSTANT ENUM
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------*/
/* UF2
 *------------------------------------------------------------------*/
void read_block(uint32_t block_no, uint8_t *data);
int write_block(uint32_t block_no, uint8_t *data, bool quiet/*, WriteState *state*/);

/*------------------------------------------------------------------*/
/* API
 *------------------------------------------------------------------*/
void msc_uf2_init(void)
{

}

void msc_uf2_mount(void)
{

}

void msc_uf2_umount(void)
{

}

//--------------------------------------------------------------------+
// tinyusb callbacks
//--------------------------------------------------------------------+
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const* ptr = NULL;
  uint16_t len = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_TEST_UNIT_READY:
      // Command that host uses to check our readiness before sending other commands
      len = 0;
    break;

    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      len = 0;
    break;

    case SCSI_CMD_START_STOP_UNIT:
      // Host try to eject/safe remove/powerof us. We could safely disconnect with disk storage, or go into lower power
      // scsi_start_stop_unit_t const * cmd_start_stop = (scsi_start_stop_unit_t const *) scsi_cmd
      len = 0;
    break;

    default:
      // negative is error -> Data stage is STALL, status = failed
      return -1;
  }

  // return len must not larger than bufsize
  TU_ASSERT( bufsize >= len );

  if ( ptr && len )
  {
    if(in_xfer)
    {
      memcpy(buffer, ptr, len);
    }else
    {
      // SCSI output
    }
  }

  return len;
}

/*------------------------------------------------------------------*/
/* Tinyusb Flash READ10 & WRITE10
 *------------------------------------------------------------------*/
int32_t tud_msc_read10_cb (uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  // since we return block size each, offset should always be zero
  TU_ASSERT(offset == 0, -1);

  uint32_t count = 0;

  while ( count < bufsize )
  {
    read_block(lba, buffer);

    lba++;
    buffer += 512;
    count  += 512;
  }

  return count;
}

int32_t tud_msc_write10_cb (uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  uint32_t count = 0;
  int wr_ret;

  while ( (count < bufsize) && ((wr_ret = write_block(lba, buffer, false)) > 0) )
  {
    lba++;
    buffer += 512;
    count  += 512;
  }

  // Consider non-uf2 block write as successful
  return  (wr_ret < 0) ? bufsize : count;
}

#endif