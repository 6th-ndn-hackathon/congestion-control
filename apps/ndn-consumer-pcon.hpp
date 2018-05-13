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

#ifndef NDN_CONSUMER_PCON_H
#define NDN_CONSUMER_PCON_H

#include "ndn-consumer-window.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief Ndn application for sending out Interest packets (window-based)
 *
 * !!! ATTENTION !!! This is highly experimental and relies on experimental features of the
 *simulator.
 * Behavior may be unpredictable if used incorrectly.
 */
class ConsumerPCON : public ConsumerWindow {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   */
  ConsumerPCON() {
    std::cout << "Started Consumer PCON\n";
    ConsumerWindow();
  }

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject) override;

  virtual void
  OnTimeout(uint32_t sequenceNumber) override;

//  virtual void
//  WillSendOutInterest(uint32_t sequenceNumber);

private:
  void windowDecrease(bool setInitialWindow);

  void windowIncrease();

  void
  cubicIncrease(){
    assert(false);
  };

  void
  cubicDecrease(){
    assert(false);
  };

  void
  bicIncrease(){
    assert(false);
  };

  void
  bicDecrease(bool resetToInitial = false){
    assert(false);
  };



private:
  const double BETA {0.5};

//  double m_cwnd{2};
  double m_sstresh{std::numeric_limits<int32_t>::max()};


  // Variables for conservative window adaptation.
  uint32_t m_highData;
  double m_recoveryPoint;


  /* TCP CUBIC Parameters */
  static constexpr double CUBIC_C = 0.4;
  static constexpr double MAX_SSTRESH = 10; /* For limited slow start */

  static const shared_ptr<std::ofstream> m_osOutput;

//  double m_cubic_k;
  double m_cubic_wmax;
  double m_cubic_last_wmax;
  time::steady_clock::TimePoint m_cubic_lastDecrease;

  ns3::Time m_ccStopTime {ns3::Time::Max()};

private:
  /* TCP BIC Parameters */
  // Regular TCP behavior (including ss) until this window size
  static constexpr int BIC_LOW_WINDOW = 14;

  // Should be between 8 and 64.
  static constexpr int BIC_MAX_INCREMENT = 16;

  // BIC variables:
  double bic_min_win; /* increase cwnd by 1 after ACKs */
  double bic_max_win; /* last maximum snd_cwnd */
  double bic_target_win; /* the last snd_cwnd */
  double bic_ss_cwnd;
  double bic_ss_target;
  bool m_is_bic_ss;

};

} // namespace ndn
} // namespace ns3

#endif
