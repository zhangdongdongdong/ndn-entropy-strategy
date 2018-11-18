/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@cs.ucla.edu>
 */


#ifndef NDNSIM_ENTROPY_ROUTE_H
#define NDNSIM_ENTROPY_ROUTE_H

#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-limits.h"
#include "ns3/traced-value.h"


#include "ns3/int64x64.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/lexical_cast.hpp>  

#include "ns3/ndn-pit-entry.h"
#include "green-yellow-red.h"
#include "ns3/log.h"




namespace ns3 {
namespace ndn {
namespace fw {

class FaceMetricWithEntroy
{

public:
  /**
   * \brief Metric constructor
   *
   * \param face Face for which metric
   * \param cost Initial value for routing cost
   */
  
  FaceMetricWithEntroy (Ptr<Face> face, fib::FaceMetric::Status  status,Time sRtt, int32_t routingCost ,double PI)
    : m_face (face)
    , m_status (fib::FaceMetric::NDN_FIB_YELLOW)
    , m_sRtt (sRtt)
    , m_pi (PI)  //ZY
    , m_routingCost(routingCost)
    , m_status_formal   (double(0.0))
    , m_sRtt_formal (double(0.0))
    , m_pi_formal (double(0.0)) //ZY
    , m_score (double(0.0))
  {
    m_status.Set (status);
  }

  /**
   * \brief Comparison operator used by boost::multi_index::identity<>
   */
  bool
  operator< (const FaceMetricWithEntroy &fm) const { return *m_face < *fm.m_face; } // return identity of the face

  /**
   * @brief Comparison between FaceMetricWithEntroy and Face
   */
  bool
  operator< (const Ptr<Face> &face) const { return *m_face < *face; }

  /**
   * @brief Return Face associated with FaceMetricWithEntroy
   */
  Ptr<Face>
  GetFace () const { return m_face; }

  /**
   * \brief Recalculate smoothed RTT and RTT variation
   * \param rttSample RTT sample
   */
  void
  UpdateRtt (const Time &rttSample);

  /**
   * @brief Get current status of FIB entry
   */
  fib::FaceMetric::Status
  GetStatus () const
  {
    return m_status;
  }

  /**
   * @brief Set current status of FIB entry
   */
  void
  SetStatus (fib::FaceMetric::Status status)
  {
    m_status= status;
  }
  
  /**
   * @brief Get current routing cost
   */
  int32_t
  GetRoutingCost () const
  {
    return m_routingCost;
  }

  /**
   * @brief Set routing cost
   */
  void
  SetRoutingCost (int32_t routingCost)
  {
    m_routingCost = routingCost;
  }


  /**
   * @brief Get current estimate for smoothed RTT value
   */
  Time
  GetSRtt () const
  {
    return m_sRtt;
  }
  
  /**
   * @brief Set current status of FIB entry
   */
  void
  SetSRtt (Time rtt)
  {
    m_sRtt=rtt;
  }

  //zy start
  double
  GetPI () const
  {
    return m_pi;
  }
  
  /**
   * @brief Set current status of FIB entry
   */
  void
  SetPI (double PI)
  {
    m_pi = PI;

  } 

  //zy end
  
  
   /**
   * @brief Get current m_status_formal
   */
  double
  GetStatusFormal () const
  {
    return m_status_formal;
  }

   /**
   * @brief Set m_status_formal
   */
  void
  SetStatusFormal (double status_formal)
  {
    m_status_formal = status_formal;
  }
  
   /**
   * @brief Get current m_sRtt_formal
   */
  double
  GetSRttFormal () const
  {
    return m_sRtt_formal;
  }

   /**
   * @brief Set m_sRtt_formal
   */
  void
  SetSRttFormal (double sRtt_formal)
  {
    m_sRtt_formal = sRtt_formal;
  }
  

  //zy start
  double
  GetPIFormal () const
  {
    return m_pi_formal;
  }  
  /**
   * @brief Set current status of FIB entry
   */
  void
  SetPIFormal (double pi_formal)
  {
    m_pi_formal = pi_formal;
  } 
  //zy end

  
   /**
   * @brief Get current m_score
   */
  double
  GetScore () const
  {
    return m_score;
  }

   /**
   * @brief Set m_score
   */
  void
  SetScore (double score)
  {
    m_score = score;
  }
  
  
private:
  Ptr<Face> m_face; ///< Face

  TracedValue<fib::FaceMetric::Status> m_status; ///< \brief Status of the next hop:
				///<		- NDN_FIB_GREEN
				///<		- NDN_FIB_YELLOW
				///<		- NDN_FIB_RED
  Time m_sRtt;         ///< \brief smoothed round-trip time; the default unit is ns(Time::NS) which is defined in SetDefaultNsResolution() in time.cc
  double m_pi;
  int32_t m_routingCost; ///< \brief routing protocol cost (interpretation of the value depends on the underlying routing protocol)
  double m_status_formal;	///<\brief this is used to store the value for m_status after standard solving.
  double m_sRtt_formal;		///<\brief this is used to store the value for m_sRtt after standard solving.
  double m_pi_formal;
  double m_score;		///<\brief this is used to store the finally score for ranking
};
  


/// @cond include_hidden
class i_status {};
class i_srtt {};
class i_nth {};
class i_pi {};
class i_score {};
/// @endcond


/**
 * @ingroup ndn-fib
 * @brief Typedef for indexed face container of Entry
 *
 * Currently, there are 2 indexes:
 * - by face (used to find record and update metric)
 * - by metric (face ranking)
 * - random access index (for fast lookup on nth face). Order is
 *   maintained manually to be equal to the 'by metric' order
 */
struct FaceMetricWithEntroyContainer
{
  /// @cond include_hidden
  typedef boost::multi_index::multi_index_container<
    FaceMetricWithEntroy,
    boost::multi_index::indexed_by<
   
	// List of available faces ordered by (status by default)
	boost::multi_index::ordered_non_unique<
	  boost::multi_index::tag<i_status>,
	  boost::multi_index::const_mem_fun<FaceMetricWithEntroy,fib::FaceMetric::Status,&FaceMetricWithEntroy::GetStatus>
	>,
	
	// List of available faces ordered by (strr by default)
	boost::multi_index::ordered_non_unique<
	  boost::multi_index::tag<i_srtt>,
	  boost::multi_index::const_mem_fun<FaceMetricWithEntroy,Time,&FaceMetricWithEntroy::GetSRtt>
	  //,std::greater<Time>
	>,

    // List of available faces ordered by (pi by default)
  boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<i_pi>,
    boost::multi_index::const_mem_fun<FaceMetricWithEntroy,double,&FaceMetricWithEntroy::GetPI>
  >,
    
	// List of available faces ordered by (score reversely,status by default)
	boost::multi_index::ordered_non_unique<
	  boost::multi_index::tag<i_score>,
	  boost::multi_index::composite_key<
	    FaceMetricWithEntroy,
	    boost::multi_index::const_mem_fun<FaceMetricWithEntroy,double,&FaceMetricWithEntroy::GetScore>,
	    boost::multi_index::const_mem_fun<FaceMetricWithEntroy,fib::FaceMetric::Status,&FaceMetricWithEntroy::GetStatus>, 
	    
	    boost::multi_index::const_mem_fun<FaceMetricWithEntroy,int32_t,&FaceMetricWithEntroy::GetRoutingCost>
	  >,
	boost::multi_index::composite_key_compare<
	  std::greater<double>,   // score sorted reverse
	  std::less<fib::FaceMetric::Status>, // status sorted by default
	  
	  std::less<int32_t> // routingCost sorted by default
	 >
      >,

      // To optimize nth candidate selection (sacrifice a little bit space to gain speed)
      //Maybe This is not needed
      boost::multi_index::random_access<
        boost::multi_index::tag<i_nth>
      >
    >
   > type;
  /// @endcond
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  
/**
 * @ingroup ndn-fw
 * @brief Entropy route strategy
 */
class EntropyStrategy :
    public GreenYellowRed
{
private:
  typedef GreenYellowRed super;

public:
  static TypeId
  GetTypeId ();

  /**
   * @brief Helper function to retrieve logging name for the forwarding strategy
   */
  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  EntropyStrategy ();
        
  // from super
  virtual bool
  DoPropagateInterest (Ptr<Face> incomingFace,
                       Ptr<const Interest> interest,
                       Ptr<pit::Entry> pitEntry);
protected:
  static LogComponent g_log;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_ENTROPY_ROUTE_H
