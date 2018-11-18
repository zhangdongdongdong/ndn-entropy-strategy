#include <ns3/ndnPlaneFow-module.h>
#include <ns3/core-module.h>

#include <ns3/ndnSIM-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/l2-rate-tracer.h>

// for ndn::AppDelayTracer
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

#include <boost/lexical_cast.hpp>


using namespace std;
using namespace boost;

NS_LOG_COMPONENT_DEFINE ("Experiment");

#define _LOG_INFO(x) NS_LOG_INFO(x)

int
main (int argc, char *argv[])
{
  _LOG_INFO ("Begin congestion-pop scenario (NDN)");

  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
  // Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("60"));

  Config::SetDefault ("ns3::ndn::RttEstimator::MaxRTO", StringValue ("1s"));
  
  Config::SetDefault ("ns3::ndn::ConsumerWindow::Window", StringValue ("1"));
  // Config::SetDefault ("ns3::ndn::ConsumerWindow::InitialWindowOnTimeout", StringValue ("false")); // irrelevant

  uint32_t run = 1;
  CommandLine cmd;
  cmd.AddValue ("run", "Simulation run", run);
  cmd.Parse (argc, argv);

  CongestionZoomExperiment experiment;
  string prefix = "/home/duoduo/new-ndnsim/experimentResult/test3-180113/multiple-congestion-zoom-ndn-entropyRouting-" + lexical_cast<string> (run)+"-";

  _LOG_INFO ("NDN experiment");
  experiment.ConfigureTopology ("/home/duoduo/new-ndnsim/topologies/test-zoom.txt");
  experiment.InstallNdnStack ("EntropyStrategy");
  
  double frequency= run*100.0;
  
  experiment.AddNdnApplications (frequency,8,StringValue("1000"));

  //boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
    //rateTracers = ndn::L3RateTracer::InstallAll (prefix + "rate-trace.log", Seconds (0.5));
   ndn::L3RateTracer::InstallAll (prefix + "rate-trace.log", Seconds (1.0));
   ndn::AppDelayTracer::InstallAll (prefix + "app-delays-trace.log");
   L2RateTracer::InstallAll (prefix+"drop-trace.txt", Seconds (1.0));
   //ndn::AppDelayTracer::InstallAll (prefix + "consumers-seqs.log");

  experiment.Run (Seconds (50.0));

  return 0;
}
