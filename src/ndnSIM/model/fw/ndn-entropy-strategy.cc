/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
// custom-strategy.cc

#include "ndn-entropy-strategy.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/core-module.h"

#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <math.h>

#include <stdlib.h>
#include <time.h>
#include <iostream>
using namespace std;



#define EPSILON 0.000001
namespace ll = boost::lambda;

//NS_LOG_COMPONENT_DEFINE ("ndn.fw.GreenYellowRed.EntropyStrategy");

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (EntropyStrategy);

LogComponent EntropyStrategy::g_log = LogComponent (EntropyStrategy::GetLogName ().c_str ());


//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////

struct FaceMetricWithEntroyByStatus
{
  typedef FaceMetricWithEntroyContainer::type::index<i_status>::type 
  type;
};


//This function is used to fetch the LogName. Since super::GetLogName () is defined in ndn-forwarding-strategy in which it returns "ndn.fw",
//so the log name for this model is "ndn.fw.EntropyStrategy".
//When we want to see the log of this model, use NS_LOG=ndn.fw.EntropyStrategy please.
std::string
EntropyStrategy::GetLogName ()
{
  return super::GetLogName ()+".EntropyStrategy";
}


TypeId
EntropyStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::EntropyStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <EntropyStrategy> ()
    ;
  return tid;
}

EntropyStrategy::EntropyStrategy ()
{
  NS_LOG_INFO ("intiate EntropyStrategy");
}

bool
EntropyStrategy::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> interest,
                                Ptr<pit::Entry> pitEntry)
{
  NS_LOG_INFO ("Begin solving forwarding");
  FaceMetricWithEntroyContainer::type m_faces_withEntroy;
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////pre-solving
  NS_LOG_DEBUG ("Trying to translate fib to fib_with_entroy");
  
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
  {
    m_faces_withEntroy.insert( FaceMetricWithEntroy( metricFace.GetFace(),metricFace.GetStatus(),metricFace.GetSRtt(),metricFace.GetRoutingCost(),metricFace.GetWeight() ) );
     
  }
  
  NS_LOG_DEBUG ("the interest is " << *interest<<" the size of fib_with_entroy = "<<m_faces_withEntroy.size());
  //NS_LOG_DEBUG ("the interest is " << *interest<<" the size of fib = "<<pitEntry->GetFibEntry ()->m_faces.size()); //zy
  
  //data standard pre-process
  if( m_faces_withEntroy.size() > 1 )
  {
    int status_min = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().begin()->GetStatus () );
    int status_max = boost::lexical_cast<int>( m_faces_withEntroy.get<i_status>().rbegin()->GetStatus () );
    int64_t srtt_min = m_faces_withEntroy.get<i_srtt>().begin()->GetSRtt().ToInteger(Time::NS);
    int64_t srtt_max = m_faces_withEntroy.get<i_srtt>().rbegin()->GetSRtt().ToInteger(Time::NS);

    //author zy
    // double pi_min = m_faces_withEntroy.get<i_pi>().begin()->GetPI();
    // double pi_max = m_faces_withEntroy.get<i_pi>().rbegin()->GetPI();
    // double pi_gap = (pi_min == pi_max) ? 1.0 : (pi_max - pi_min);
    // double m_pi_sum = 0.0;
    //
    
    int status_gap = (status_min == status_max) ? 1 : (status_max - status_min);
    int64_t srtt_gap = (srtt_min == srtt_max) ? 1 : (srtt_max - srtt_min);
    
    double efficacy_coefficient = 0.95;
    
    double status_sum=0.0;
    double m_sRtt_sum=0.0;
    
    NS_LOG_INFO("status_max="<<status_max<<",status_min="<<status_min<<	",srtt_max="<<srtt_max<<",srtt_min="<<srtt_min
      );
    //data standard process
    //FaceMetricWithEntroyByStatus must be corresponding to i_status
    
// (3)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status>().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status>().end();
       metricFacewithEntroy++)
    {
      double status_formal = ( status_max -  boost::lexical_cast<int>( metricFacewithEntroy->GetStatus()) ) * efficacy_coefficient / 
	status_gap + (1.0-efficacy_coefficient);
      
      double m_sRtt_formal = ( srtt_max -   metricFacewithEntroy->GetSRtt ().ToInteger(Time::NS) ) * efficacy_coefficient / 
	srtt_gap + (1.0-efficacy_coefficient);

  //     double m_pi_formal = (pi_max - metricFacewithEntroy->GetPI()) * efficacy_coefficient /
  // pi_gap + (1.0-efficacy_coefficient);
      
	
      status_sum+=status_formal;
      m_sRtt_sum+=m_sRtt_formal;
      //m_pi_sum += m_pi_formal;
	
      m_faces_withEntroy.modify (metricFacewithEntroy,
                      ll::bind (&FaceMetricWithEntroy::SetStatusFormal, ll::_1, status_formal));

      m_faces_withEntroy.modify (metricFacewithEntroy,
                      ll::bind (&FaceMetricWithEntroy::SetSRttFormal, ll::_1, m_sRtt_formal));

      //m_faces_withEntroy.modify (metricFacewithEntroy, 
                      //ll::bind (&FaceMetricWithEntroy::SetPIFormal, ll::_1, m_pi_formal));
      
    }
    
    double status_entropy_value=0.0;
    double m_sRtt_entropy_value=0.0;
    //double m_pi_entropy_value=0.0;
   
    //calculate ratio Pij
    
// (4)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status>().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status>().end();
       metricFacewithEntroy++)
    {
      double status_ratio = metricFacewithEntroy->GetStatusFormal() / status_sum;
      double m_sRtt_ratio = metricFacewithEntroy->GetSRttFormal() / m_sRtt_sum;
     // double m_pi_ratio = metricFacewithEntroy->GetPIFormal() / m_pi_sum;

      
      //we use loge(x) to calculate entropy value
      status_entropy_value += status_ratio * (log(status_ratio));
      m_sRtt_entropy_value += m_sRtt_ratio * (log(m_sRtt_ratio));
     // m_pi_entropy_value += m_pi_ratio * (log(m_pi_ratio));

	
      m_faces_withEntroy.modify (metricFacewithEntroy,
                      ll::bind (&FaceMetricWithEntroy::SetStatusFormal, ll::_1, status_ratio));

      m_faces_withEntroy.modify (metricFacewithEntroy,
                      ll::bind (&FaceMetricWithEntroy::SetSRttFormal, ll::_1, m_sRtt_ratio));

     // m_faces_withEntroy.modify (metricFacewithEntroy,
                     // ll::bind (&FaceMetricWithEntroy::SetPIFormal, ll::_1, m_pi_ratio));

    }
    
    //calculate entropy value for each col
    double k = -1.0 / (log(m_faces_withEntroy.size()));
    NS_LOG_INFO("k="<<k); 
    //NS_LOG_INFO("status_entropy_value before multiple k="<<status_entropy_value<< ",m_sRtt_entropy_value before multiple k="<<m_sRtt_entropy_value<<  ",m_pi_entropy_value before multiple k="<<m_pi_entropy_value
      //);
    status_entropy_value = k * status_entropy_value;
    m_sRtt_entropy_value = k * m_sRtt_entropy_value;
    //m_pi_entropy_value = k * m_pi_entropy_value;

    //NS_LOG_INFO("status_entropy_value="<<status_entropy_value<<",m_sRtt_entropy_value="<<m_sRtt_entropy_value<< ",m_pi_entropy_value="<<m_pi_entropy_value
     // );   //zy
    
    //calculate entropy redundancy rate
    double  status_entropy_redundancy_rate = 1 - status_entropy_value;
    double  m_sRtt_entropy_redundancy_rate = 1 - m_sRtt_entropy_value;
   // double  m_pi_entropy_redundancy_rate = 1 - m_pi_entropy_value;


    //NS_LOG_INFO("status_entropy_redundancy_rate="<<status_entropy_redundancy_rate<<",m_sRtt_entropy_redundancy_rate="<<m_sRtt_entropy_redundancy_rate<< ",m_pi_entropy_redundancy_rate="<<m_pi_entropy_redundancy_rate

      //); 
    
    //need to judge whether both rate is equal to 0 in special case(emerge in cold start)
    if ( fabs( status_entropy_redundancy_rate - m_sRtt_entropy_redundancy_rate) < EPSILON &&  fabs( status_entropy_redundancy_rate - 0.0) < EPSILON) 
    {
      status_entropy_redundancy_rate=1.0;
      m_sRtt_entropy_redundancy_rate=status_entropy_redundancy_rate;
     //m_pi_entropy_redundancy_rate=status_entropy_redundancy_rate;
    }


    //cout<<'a'<<status_entropy_redundancy_rate<<','<<m_sRtt_entropy_redundancy_rate<<','<<m_pi_entropy_redundancy_rate<<';';

    //calculate weight
    //double sum_weight = status_entropy_redundancy_rate + m_sRtt_entropy_redundancy_rate + m_pi_entropy_redundancy_rate;
    double sum_weight = status_entropy_redundancy_rate + m_sRtt_entropy_redundancy_rate ;
    double status_weight = status_entropy_redundancy_rate / sum_weight;
    double m_sRtt_weight = m_sRtt_entropy_redundancy_rate / sum_weight;
   // double m_pi_weight = m_pi_entropy_redundancy_rate / sum_weight;
    
    //cout<<"v1:v2"<<status_weight<<", "<<m_sRtt_weight<<';'<<endl;



    
    //calculate score
 
// (5)
    for (FaceMetricWithEntroyByStatus::type::iterator metricFacewithEntroy = m_faces_withEntroy.get<i_status> ().begin();
       metricFacewithEntroy !=  m_faces_withEntroy.get<i_status> ().end ();
       metricFacewithEntroy++)
    {
      double score = status_weight * metricFacewithEntroy->GetStatusFormal() +
			m_sRtt_weight * metricFacewithEntroy->GetSRttFormal() ;
  
      //+m_pi_weight * metricFacewithEntroy->GetPIFormal();
     

      m_faces_withEntroy.modify (metricFacewithEntroy,
                      ll::bind (&FaceMetricWithEntroy::SetScore, ll::_1, score));
    }
    
    NS_LOG_INFO("status_weight="<<status_weight<<",m_sRtt_weight="<<m_sRtt_weight
      ); 
    
  }
  
  NS_LOG_DEBUG ("Trying to show fib_with_entroy");
  NS_LOG_INFO("begin =============================================================================================");
  BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
  {
    
    NS_LOG_INFO("metricFacewithEntroy.m_status="<<metricFacewithEntroy.GetStatus ()<<
      ",metricFacewithEntroy.m_sRtt="<<metricFacewithEntroy.GetSRtt ()<<
     // ",metricFacewithEntroy.m_pi="<<metricFacewithEntroy.GetPI()<<
	",metricFacewithEntroy.m_status_formal="<<metricFacewithEntroy.GetStatusFormal() <<
	",metricFacewithEntroy.m_sRtt_formal="<<metricFacewithEntroy.GetSRttFormal() <<
 // ",metricFacewithEntroy.m_pi_formal="<<metricFacewithEntroy.GetPIFormal() <<
	",metricFacewithEntroy.m_score="<<metricFacewithEntroy.GetScore()
      ); 
    
    //NS_LOG_INFO("only once");
    //break;
     
  }
  NS_LOG_INFO("end =============================================================================================");
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////forwarding
  
  NS_LOG_FUNCTION (this << interest->GetName ());
  
  int propagatedCount = 0;
  
  /*
  BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
  {
    //NS_LOG_DEBUG ("Trying " << boost::cref(metricFacewithEntroy));
    //because we use score as first choice to select face, so this may lead to the fact that red face rank high in some special case.
    if (metricFacewithEntroy.GetStatus () == fib::FaceMetric::NDN_FIB_RED)
        continue;
    if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
        {
          continue;
        }

      propagatedCount++;
      break; // do only once
     
  }
  */
  
  //produce random number 
  //This can not work with a error, and I don't know why
  /*double p;
  srand((unsigned)time(NULL));                   //时间种子
  double a=0.00000000001;              //先做double a种子
  p=a*(rand()%10000000000);*/
  
  UniformVariable x (0.0,1.0);
  double p =x.GetValue ();
  double total_score=0.0;
  double per_probability=0.0;
  
  double gamma=3;
  double delta=1; 
  
  // 
  BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
  {
    NS_LOG_INFO("error =======dynamic_score====="<<pow(metricFacewithEntroy.GetScore (),gamma)<<"=============error");
    NS_LOG_INFO("error =======GetRoutingCost+1====="<<(metricFacewithEntroy.GetRoutingCost ()+1)<<"=============error");
    NS_LOG_INFO("error =======static_route====="<<pow(1.0/(metricFacewithEntroy.GetRoutingCost ()+1),delta)<<"=============error");
   
    total_score+= pow(metricFacewithEntroy.GetScore (),gamma) * pow(1.0/(metricFacewithEntroy.GetRoutingCost ()+1),delta) ; //modified
    
    NS_LOG_INFO("error =======total_score====="<<total_score<<"=============error");
     
  }
  
  if(fabs( total_score - 0.0) < EPSILON)
  {
    NS_LOG_INFO("error =======total_score====="<<total_score<<"=============error");
    BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
    {
      //NS_LOG_DEBUG ("Trying " << boost::cref(metricFacewithEntroy));
      //because we use score as first choice to select face, so this may lead to the fact that red face rank high in some special case.
      if (metricFacewithEntroy.GetStatus () == fib::FaceMetric::NDN_FIB_RED)
	  continue;
      if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
	  {
	    continue;
	  }

	propagatedCount++;
	break; // do only once
      
    }
    
  }
  else
  {
    NS_LOG_INFO("total_score =============="<<total_score);
    BOOST_FOREACH (const FaceMetricWithEntroy &metricFacewithEntroy, m_faces_withEntroy.get<i_score> ())
    {
      per_probability += pow(metricFacewithEntroy.GetScore (),gamma) * pow(1.0/(metricFacewithEntroy.GetRoutingCost ()+1),delta) / total_score; //modified
      if(per_probability < p)
      {
	continue;
      }
      if (!TrySendOutInterest (inFace, metricFacewithEntroy.GetFace (), interest, pitEntry))
      {
	continue;
      }
      propagatedCount++;
      break; // do only once;
      
    }
    
  }
  
 
  
  
  // No real need to call parent's (green-yellow-red's strategy) method, since it is incorporated
  // in the logic of the EntropyStrategy strategy
  //
  // // Try to work out with just green faces
  // bool greenOk = super::DoPropagateInterest (inFace, interest, origPacket, pitEntry);
  // if (greenOk)
  //   return true;

  
  /*
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      //NS_LOG_DEBUG("metricFace.m_status="<<metricFace.GetStatusTrace ()<<",metricFace.m_sRtt="<<metricFace.GetSRtt ()); 
      NS_LOG_INFO("metricFace.m_status="<<metricFace.GetStatus ()<<",metricFace.m_sRtt="<<metricFace.GetSRtt ()<<
        ",metricFace.m_rttVar="<<metricFace.GetRttVar ()<<
	",metricFace.m_routingCost="<<metricFace.GetRoutingCost ()<<",metricFace.m_realDelay="<<metricFace.GetRealDelay () 
      ); 
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
        break;

      if (!TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
        {
          continue;
        }

      propagatedCount++;
      break; // do only once
    }
    */

  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

} // namespace fw
} // namespace ndn
} // namespace ns3

