/*
 * This file is part of the libCEC(R) library.
 *
 * Copied from TDA995x and modified for OMAP4 by Kyle Manna <kmanna@fanhattan.com>
 *
 * libCEC(R) is Copyright (C) 2011-2013 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "env.h"

#if defined(HAVE_OMAP4_API)
#include "OMAP4CECAdapterCommunication.h"

#include "lib/CECTypeUtils.h"
#include "lib/LibCEC.h"
#include "lib/platform/sockets/cdevsocket.h"
#include "lib/platform/util/StdString.h"
#include "lib/platform/util/buffer.h"

using namespace std;
using namespace CEC;
using namespace PLATFORM;

#define LIB_CEC m_callback->GetLib()

COMAP4CECAdapterCommunication::COMAP4CECAdapterCommunication(IAdapterCommunicationCallback *callback) :
    IAdapterCommunication(callback),
    m_bLogicalAddressChanged(false)
{
  CLockObject lock(m_mutex);

  m_logicalAddresses.Clear();
  m_dev = new CCDevSocket(CEC_OMAP4_PATH);
}


COMAP4CECAdapterCommunication::~COMAP4CECAdapterCommunication(void)
{
  Close();

  CLockObject lock(m_mutex);
  delete m_dev;
  m_dev = 0;
}


bool COMAP4CECAdapterCommunication::IsOpen(void)
{
  return IsInitialised() && m_dev->IsOpen();
}


bool COMAP4CECAdapterCommunication::Open(uint32_t iTimeoutMs, bool UNUSED(bSkipChecks), bool bStartListening)
{
  if(m_dev->Open(iTimeoutMs))
  {
    if (!bStartListening || CreateThread())
      return true;
  }
  return false;
}


void COMAP4CECAdapterCommunication::Close(void)
{
  StopThread(0);

  m_dev->Close();
}


std::string COMAP4CECAdapterCommunication::GetError(void) const
{
  std::string strError(m_strError);
  return strError;
}


cec_adapter_message_state COMAP4CECAdapterCommunication::Write(
  const cec_command &data, bool &bRetry, uint8_t UNUSED(iLineTimeout), bool UNUSED(bIsReply))
{
  struct cec_tx_data tx;

  tx.dest_device_id = data.destination;
  tx.initiator_device_id = data.initiator;
  tx.send_ping = data.opcode_set ? 0 : 1;
  tx.retry_count = 5; // TODO what is bRetry??
  tx.tx_cmd = data.opcode;
  tx.tx_count = data.parameters.size;

  if (tx.tx_count > sizeof(tx.tx_operand)) {
      LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: msg overflow", __func__);
      return ADAPTER_MESSAGE_STATE_ERROR;
  }

  for (int iPtr = 0; iPtr < tx.tx_count; iPtr++) {
      tx.tx_operand[iPtr] = data.parameters[iPtr];
  }

  if (m_dev->Ioctl(CEC_TRANSMIT_CMD, &tx) != 1) {
      return ADAPTER_MESSAGE_STATE_ERROR;
  }

  return ADAPTER_MESSAGE_STATE_SENT_ACKED;
}


cec_vendor_id COMAP4CECAdapterCommunication::GetVendorId(void)
{
  return cec_vendor_id(CEC_VENDOR_UNKNOWN);
}


uint16_t COMAP4CECAdapterCommunication::GetPhysicalAddress(void)
{
  uint32_t phy_addr;

  if (m_dev->Ioctl(CEC_GET_PHY_ADDR, &phy_addr) < 0) {
    LIB_CEC->AddLog(CEC_LOG_ERROR, "%s: CEC_IOCTL_GET_RAW_INFO failed !", __func__);
    return CEC_INVALID_PHYSICAL_ADDRESS;
  }

  return phy_addr;
}


cec_logical_addresses COMAP4CECAdapterCommunication::GetLogicalAddresses(void)
{
  return m_logicalAddresses;
}


bool COMAP4CECAdapterCommunication::SetLogicalAddresses(const cec_logical_addresses &addresses)
{
  m_logicalAddresses = addresses;

  memset(&m_cec_dev, 0, sizeof(m_cec_dev));

  m_cec_dev.clear_existing_device = 0x1;
  m_cec_dev.device_id = addresses.primary;

  if (m_dev->Ioctl(CEC_REGISTER_DEVICE, &m_cec_dev) < 0)
    return false;

  m_cec_dev.clear_existing_device = 0x0;
  size_t cnt = (sizeof(addresses.addresses)/sizeof(addresses.addresses[0]));
  for (size_t i = 0; i < cnt; i++)
  {
    if (addresses.IsSet((cec_logical_address)i)) {
      m_cec_dev.device_id = i;
      if (m_dev->Ioctl(CEC_REGISTER_DEVICE, &m_cec_dev) < 0)
        return false;
    }
  }

  m_bLogicalAddressChanged = true;

  return true;
}


void COMAP4CECAdapterCommunication::HandleLogicalAddressLost(cec_logical_address UNUSED(oldAddress))
{
  /* FIXME: Not sure how to handle this with respect to m_logicalAddreses */
  cec_logical_addresses addresses;
  addresses.Clear();
  addresses.Set(CECDEVICE_BROADCAST);
  SetLogicalAddresses(addresses);
}


void *COMAP4CECAdapterCommunication::Process(void)
{
  bool bHandled;
  uint32_t opcode, status;
  cec_logical_address initiator, destination;

  /* TODO: This code is ugly due to the limited functionality the TI CEC
   * kernel driver.  Some day it would nice to clean this up.
   */
  while (!IsStopped())
  {
    struct cec_rx_data rx;
    if (m_dev->Ioctl(CEC_RECV_CMD, &rx) == 0)
    {
      cec_command cmd;

      cmd.initiator = (cec_logical_address)rx.init_device_id;
      cmd.destination = (cec_logical_address)rx.dest_device_id;
      cmd.opcode = (cec_opcode)rx.rx_cmd;
      cmd.opcode_set = 1;
      cmd.parameters.size = rx.rx_count;
      for (int i = 0; i < rx.rx_count; i++)
      {
        cmd.parameters.data[i] = rx.rx_operand[i];
      }
      m_callback->OnCommandReceived(cmd);
    } else {
      /* No good way to do proper IO with the kernel, need to fix
       * kernel module at some point.
       *
       * Stop painful spinning with a (slightly) less painful sleep.
       */
      Sleep(100);
    }
  }

  return 0;
}

#endif	// HAVE_OMAP4_API
