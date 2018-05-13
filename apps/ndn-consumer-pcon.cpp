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

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerPCON");

namespace ns3 {
namespace ndn {


std::string
ConsumerPCON::getEnvVar(const std::string& name)
{
  assert(!name.empty());
  char const* val = getenv(name.c_str());
  return val == nullptr ? std::string() : std::string(val);
}


constexpr double ConsumerPCON::CUBIC_C;
//constexpr double ConsumerPCON::MAX_SSTRESH; /* For limited slow start */

constexpr int ConsumerPCON::BIC_MAX_INCREMENT;
constexpr int ConsumerPCON::BIC_LOW_WINDOW;


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
    std::cout << Simulator::Now().GetMilliSeconds() << "ms Consumer got congestion mark!\n";
    windowDecrease(false);
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
  std::cout << Simulator::Now().GetMilliSeconds() << "ms Window decrease: "
      << m_window << " -> " << m_window * BETA << "\n";
  std::cout.flush();

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
  m_window = std::max(m_window, double(m_initialWindow));
}


void
ConsumerPCON::OnTimeout(uint32_t sequenceNumber)
{
  std::cout << Simulator::Now().GetMilliSeconds() << " ms Timeout packet " << sequenceNumber
      << "\n";
  std::cout.flush();

  windowDecrease(false);
  ConsumerWindow::OnTimeout(sequenceNumber);
}


void
ConsumerPCON::bicIncrease()
{
  if (m_window < BIC_LOW_WINDOW) {
    // Normal TCP
    if (m_window < m_sstresh) {
      // Old ERROR: Bug of <=
//      std::cout << "Slow start increase!\n";
      m_window = m_window + 1;
    }
    else {
//      std::cout << "Cwnd: " << m_window << " <- " << m_window + 1.0 / m_window << "\n";
      m_window = m_window + 1.0 / m_window;
    }
  }
  //            std::cout << "BIC Increase, cwnd: " << m_window << ", bic_target_win: "
  // << bic_target_win << "\n";
  else if (m_is_bic_ss == false) { // bin. increase
    if (bic_target_win - m_window < BIC_MAX_INCREMENT) { // binary search
      m_window += (bic_target_win - m_window) / m_window;
    }
    else {
      m_window += BIC_MAX_INCREMENT / m_window; // additive increase
    }
    // FIX for equal double values.
    if (m_window + 0.00001 < bic_max_win) {
      //                std::cout << "3 Cwnd: " << m_window << ", bic_max_win: " <<
      // bic_max_win << "\n";
      bic_min_win = m_window;
      //                std::cout << "bic_max_win: " << bic_max_win << ", bic_min_win: "
      // << bic_min_win << "\n";
      bic_target_win = (bic_max_win + bic_min_win) / 2;
    }
    else {
      m_is_bic_ss = true;
      bic_ss_cwnd = 1;
      bic_ss_target = m_window + 1.0;
      bic_max_win = std::numeric_limits<int>::max();
    }
  }
  // BICslow start
  else {
    m_window += bic_ss_cwnd / m_window;
    if (m_window >= bic_ss_target) {
      bic_ss_cwnd = 2 * bic_ss_cwnd;
      bic_ss_target = m_window + bic_ss_cwnd;
    }
    if (bic_ss_cwnd >= BIC_MAX_INCREMENT) {
      m_is_bic_ss = false;
    }
  }
}

void
ConsumerPCON::bicDecrease(bool resetToInitial)
{
  // BIC Decrease
  if (m_window >= BIC_LOW_WINDOW) {
    auto prev_max = bic_max_win;
    bic_max_win = m_window;
    m_window = m_window * BETA;
    bic_min_win = m_window;
    if (prev_max > bic_max_win) { //Fast. Conv.
      bic_max_win = (bic_max_win + bic_min_win) / 2;
    }
    bic_target_win = (bic_max_win + bic_min_win) / 2;
  } else {
    // Normal TCP Decrease:
    m_sstresh = m_window * BETA;
    m_window = m_sstresh;
  }

  if (resetToInitial) {
    // Does this work for TCP BIC?
    assert(false);
    m_sstresh = m_window * BETA;
    m_window = m_initialWindow;
  }
}


void
ConsumerPCON::cubicIncrease()
{
  // 1. Time since last congestion event in Seconds
  const double t = time::duration_cast<time::microseconds>(
      time::steady_clock::now() - m_cubic_lastDecrease).count() / 1000000.0; // @suppress("Method cannot be resolved")

  // 2. Time it takes to increase the window to cubic_wmax
  // K = cubic_root(W_max*(1-beta_cubic)/C) (Eq. 2)
  const double k = std::cbrt(m_cubic_wmax * (1 - BETA) / CUBIC_C);
  // Or use m_cubic_k

  // 3. Target: W_cubic(t) = C*(t-K)^3 + W_max (Eq. 1)
  const double w_cubic = CUBIC_C * std::pow(t - k, 3) + m_cubic_wmax;

  // 4. Estimate of Reno Increase (Currently Disabled)
  //  const double rtt = m_rtt->GetCurrentEstimate().GetSeconds();
  //  const double w_est = m_cubic_wmax*m_beta + (3*(1-m_beta)/(1+m_beta)) * (t/rtt);
  constexpr double w_est {0.0};


  // Actual adaptation
  if (m_window < m_sstresh) {
    // TODO: Maybe enable limited slow start:
//    if (m_window < MAX_SSTRESH) {
      m_window += 1;
//    } else {
//      // Limited Slow start:
//      double K = m_window / (0.5 * MAX_SSTRESH);
//      m_window += 1.0 / K;
//    }
  }
  else {
    // w_max must be larger than 0:
    assert(m_cubic_wmax > 0);

    double cubic_increment = std::max(w_cubic, w_est) - m_window;
    // Cubic increment must be positive:
    if (cubic_increment < 0) {
      // TODO: Should this be set to 0?
//      std::cout << "Increment: " << cubic_increment << "\n";
      cubic_increment = 0.0;
    }
    m_window += cubic_increment/m_window;
  }

}


void
ConsumerPCON::cubicDecrease()
{
  // Fast convergence (further reduce w_max):
  const bool CUBIC_FAST_CONV = true;

  // In percent
  const double FAST_CONV_DIFF = 1.0;

  // A flow remembers the last value of W_max,
  // before it updates W_max for the current congestion event.

  // Current w_max < last_wmax
  if (CUBIC_FAST_CONV && m_window < m_cubic_last_wmax * (1 - FAST_CONV_DIFF/100)) {

    // m_cubic_wmax = cwnd_*(1+cubic_beta_)/2;
    m_cubic_last_wmax = m_window;
    m_cubic_wmax = m_window * (1.0 + CUBIC_BETA) / 2.0; // reduce by 0.9
  }
  else {
    // Save old cwnd as w_max:
    // m_cubic_wmax = cwnd_;
    m_cubic_last_wmax = m_window;
    m_cubic_wmax = m_window;
  }

  m_sstresh = m_window * CUBIC_BETA;
  m_sstresh = std::max(m_sstresh, double(m_initialWindow)); // Why is the minimum 2?
  m_window = m_sstresh;

  m_cubic_lastDecrease = time::steady_clock::now();
}






} // namespace ndn
} // namespace ns3
