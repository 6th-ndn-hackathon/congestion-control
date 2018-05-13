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

#include <boost/algorithm/string.hpp>


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
  ConsumerPCON() :
      CC_ALGORITHM {boost::to_upper_copy(getEnvVar("SCEN_NAME"))}
  {
    assert(!CC_ALGORITHM.empty());
    std::cout << "Started Consumer PCON, Algorithm: " << CC_ALGORITHM << "\n";
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
  void windowDecrease();

  void windowIncrease();

  void
  cubicIncrease();

  void
  cubicDecrease();

  void
  bicIncrease();

  void
  bicDecrease(bool resetToInitial = false);

  static std::string
  getEnvVar(const std::string& name);


private:
  const double BETA {0.5};
  const double CUBIC_BETA {0.8};

  //  double m_cwnd{2.0};
  double m_sstresh{std::numeric_limits<int32_t>::max()};

  // Variables for conservative window adaptation.
  uint32_t m_highData {0};
  double m_recoveryPoint {0};


  /* TCP CUBIC Parameters */
  static constexpr double CUBIC_C  = 0.4;
//  static constexpr double MAX_SSTRESH  = 10; /* For limited slow start */

  static const shared_ptr<std::ofstream> m_osOutput;

//  double m_cubic_k;
  double m_cubic_wmax{0};
  double m_cubic_last_wmax{0};
  time::steady_clock::TimePoint m_cubic_lastDecrease{time::steady_clock::now()};

  ns3::Time m_ccStopTime {ns3::Time::Max()};

private:
  /* TCP BIC Parameters */
  // Regular TCP behavior (including ss) until this window size
  static constexpr int BIC_LOW_WINDOW = 14;

  // Should be between 8 and 64.
  static constexpr int BIC_MAX_INCREMENT  = 16;

  const std::string CC_ALGORITHM;

  // BIC variables:
  double bic_min_win{0}; /* increase cwnd by 1 after ACKs */
  double bic_max_win{std::numeric_limits<int32_t>::max()}; /* last maximum snd_cwnd */
  double bic_target_win{0}; /* the last snd_cwnd */
  double bic_ss_cwnd{0};
  double bic_ss_target{0};
  bool m_is_bic_ss{false};
};

//ConsumerCC::ConsumerCC() :
//      m_initialWindow{1},
//      m_maxSize{-1},
//      m_maxMultiplier(8),
//      m_minRtoInMS(1000),
//{
//}


} // namespace ndn
} // namespace ns3

#endif
