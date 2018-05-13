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

const std::string CC_ALGORITHM {"AIMD"};

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
  Consumer::OnData(contentObject);

  // TODO: Problem in OnData!

  // Get sequence Number
  const auto seqNum = contentObject->getName().get(-1).toSequenceNumber();

  // Set highest received data to seq. number
  if (m_highData < seqNum) {
    m_highData = seqNum;
  }

  auto congMark = contentObject->getCongestionMark();
  if (congMark > 0) {
    windowDecrease(false);
    std::cout << Simulator::Now().GetMilliSeconds() << "ms Consumer got congestion mark!\n";
  }
  else {
    windowIncrease();
  }


  if (m_inFlight > 0) {
    m_inFlight--;
  }

  ScheduleNextPacket();
//  ConsumerWindow::OnData(contentObject);
}

//void
//ConsumerWindow::OnData(shared_ptr<const Data> contentObject)
//{
////  Consumer::OnData(contentObject);
//
////  m_window = m_window + 1;
//
//  if (m_inFlight > static_cast<uint32_t>(0))
//    m_inFlight--;
//  NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);
//
//  ScheduleNextPacket();
//}


void ConsumerPCON::windowIncrease()
{
  if (CC_ALGORITHM == "AIMD"){
    if (m_window < m_sstresh) {
      m_window = m_window + 1.0;
    }
    else {
      m_window += (1.0 / m_window);
    }
  }
  else if (CC_ALGORITHM == "CUBIC") {
    cubicIncrease();
  }
  else if (CC_ALGORITHM == "BIC") {
    bicIncrease();
  }
  else {
    assert((false) && "Wrong CC Algorithm!");
  }
}

void ConsumerPCON::windowDecrease(bool setInitialWindow)
{
  std::cout << Simulator::Now().GetMilliSeconds() << "ms Window decrease, new window: "
      << m_window << "\n";
  if (CC_ALGORITHM == "AIMD") {
    // Normal TCP Decrease:
    m_sstresh = m_window * BETA;
    m_window = m_sstresh;
  }
  else if (CC_ALGORITHM == "CUBIC") {
    cubicDecrease();
  }
  else if (CC_ALGORITHM == "BIC") {
    bicDecrease(setInitialWindow);
  }
  else {
    assert((false) && "Wrong CC Algorithm!");
  }

  // Cwnd can never fall below initial window!
  // TODO: Turn into double!
  m_window = std::max(m_window.Get(), double(m_initialWindow));
}


void
ConsumerPCON::OnTimeout(uint32_t sequenceNumber)
{

  std::cout << Simulator::Now().GetMilliSeconds() << " ms Timeout packet " << sequenceNumber
      << "\n";
  assert(false);

  windowDecrease(false);

  ConsumerWindow::OnTimeout(sequenceNumber);
}





} // namespace ndn
} // namespace ns3
