/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-consumer-pcon.hpp"
//#include "ns3/ptr.h"
//#include "ns3/log.h"
//#include "ns3/simulator.h"
//#include "ns3/packet.h"
//#include "ns3/callback.h"
//#include "ns3/string.h"
//#include "ns3/uinteger.h"
//#include "ns3/double.h"


NS_LOG_COMPONENT_DEFINE("ndn.ConsumerPCON");

namespace ns3 {
namespace ndn {


NS_OBJECT_ENSURE_REGISTERED(ConsumerPCON);

TypeId
ConsumerPCON::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerPCON")
      .SetGroupName("Ndn")
      .SetParent<ConsumerWindow>()
      .AddConstructor<ConsumerPCON>();

  return tid;
}


void
ConsumerPCON::OnData(shared_ptr<const Data> contentObject)
{
  auto congMark = contentObject->getCongestionMark();
  if (congMark > 0) {
    std::cout << Simulator::Now().GetMilliSeconds() << "ms Consumer got congestion mark!\n";
  }

  ConsumerWindow::OnData(contentObject);
}


void
ConsumerPCON::OnTimeout(uint32_t sequenceNumber)
{
  std::cout << Simulator::Now().GetMilliSeconds() << " ms Timeout packet " << sequenceNumber
      << "\n";

  ConsumerWindow::OnTimeout(sequenceNumber);
}





} // namespace ndn
} // namespace ns3
