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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

#include <string>

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *      +-----------+
 *      | consumer1 |
 *      +-----------+
 *                \
 *         10 Mbps \ 10ms
 *                  \
 *                 +---------+    10Mbps   +---------+    10Mbps    +----------+
 *                 | router1 | <---------> | router2 | <----------> | producer |
 *                 +---------+     10ms    +---------+     10ms     +----------+
 *                  /
 *         10 Mbps / 10ms
 *                /
 *      +-----------+
 *      | consumer2 |
 *      +-----------+
 *
 *
 * Consumers requests data from producer with filesize XMB.txt
 *
 * For every received interest, producer replies with a data packet, containing
 * actual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *    NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-wired
 *    ./waf --run "ndn-wired --cacheSize=0 --errRate=0.0"
 */

int
main(int argc, char* argv[])
{
  bool tracing = false;
  bool verbose = false;
  int cacheSize = 0;
  double errRate = 0.0;

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault ("ns3::QueueBase::MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, 102400)));
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("1ms"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue ("errRate", "Error rate per interface (0.1 = 10 percent)", errRate);
  cmd.AddValue ("cacheSize", "Content Store size", cacheSize);
  cmd.AddValue ("tracing", "Tracing true or false", tracing);
  cmd.AddValue ("verbose", "Set verbose output true or false", verbose);
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer producer;
  producer.Create (1);
  NodeContainer routers;
  routers.Create (2);
  NodeContainer consumers;
  consumers.Create (2);
  NodeContainer allNodes = NodeContainer (producer, routers, consumers);

  std::vector<NodeContainer> nodeAdjacencyList (4);
  nodeAdjacencyList[0] = NodeContainer(producer.Get(0), routers.Get(1));
  nodeAdjacencyList[1] = NodeContainer(routers.Get(1), routers.Get(0));
  nodeAdjacencyList[2] = NodeContainer(routers.Get(0), consumers.Get(0));
  nodeAdjacencyList[3] = NodeContainer(routers.Get(0), consumers.Get(1));

  // Connecting nodes using two links
  PointToPointHelper p2p;
  /*
  p2p.Install (producer.Get(0), routers.Get(1));
  p2p.Install (routers.Get(1), routers.Get(0));
  p2p.Install (routers.Get(0), consumers.Get(0));
  p2p.Install (routers.Get(0), consumers.Get(1));
   */
  std::vector<NetDeviceContainer> deviceAdjacencyList(4);
  for( int i=0; i < 4; i++)
  {
    deviceAdjacencyList[i] = p2p.Install (nodeAdjacencyList[i]);
  }

  MobilityHelper sessile;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add(Vector(-100, 100, 0)); // c1
  positionAlloc->Add(Vector(-100, -100, 0)); // c2
  positionAlloc->Add(Vector(0, 0, 0)); // r1
  positionAlloc->Add(Vector(100, 0, 0)); // r2
  positionAlloc->Add(Vector(200, 0, 0)); // server

  sessile.SetPositionAllocator (positionAlloc);
  sessile.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  sessile.Install (consumers);
  sessile.Install (routers);
  sessile.Install (producer);

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  if(cacheSize > 0)
    ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru",
                                 "MaxSize", std::to_string(cacheSize).c_str() );
  else ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPCON");
  consumerHelper.SetAttribute("Prefix", StringValue("/tmp/data"));
  consumerHelper.SetAttribute("PayloadSize", UintegerValue(1024));
  consumerHelper.SetAttribute("Size", DoubleValue(5.0));
  consumerHelper.Install(consumers.Get(0)).Start(Seconds(2.0));
  consumerHelper.Install(consumers.Get(1)).Start(Seconds(5.0));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Prefix", StringValue("/tmp"));
  producerHelper.Install(producer.Get(0)).Start(Seconds(0.1));
  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins("/tmp", producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(100.0));

  if (tracing)
  {
    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll (ascii.CreateFileStream ("ndn-consumer-producer.tr"));
    p2p.EnablePcapAll ("ndn-consumer-producer");
  }

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
