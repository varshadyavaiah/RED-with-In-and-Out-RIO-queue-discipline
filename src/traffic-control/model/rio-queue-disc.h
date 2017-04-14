#ifndef RIO_QUEUE_DISC_H
#define RIO_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class TraceContainer;

/**
 * \ingroup traffic-control
 *
 * \brief A RIO packet queue disc
 */

class RioQueueDisc : public QueueDisc
{
public:
  static TypeId GetTypeId (void);
  /**
   * \brief RioQueueDisc Constructor
   *
   * Create a RIO queue disc
   */
  RioQueueDisc ();

  /**
   * \brief Destructor
   *
   * Destructor
   */

  virtual ~RioQueueDisc ();



  /**
   * \brief Stats
   */
  typedef struct
  {
    uint32_t unforcedDrop;              //!< Early probability drops
    uint32_t forcedDrop;                //!< Forced drops, qavg > max threshold
    uint32_t qLimDrop;                  //!< Drops due to queue limits
    uint32_t unforcedMark;              //!< Early probability marks
    uint32_t forcedMark;                //!< Forced marks, qavg > max threshold
  } Stats;

  /**
   * \brief Drop types
   */
  enum
  {
    DTYPE_NONE,                //!< Ok, no drop
    DTYPE_FORCED,              //!< A "forced" drop
    DTYPE_UNFORCED,            //!< An "unforced" (random) drop
  };

  /**
   * \brief Enumeration of the modes supported in the class.
   *
   */
  enum QueueDiscMode
  {
    QUEUE_DISC_MODE_PACKETS,             /**< Use number of packets for maximum queue disc size */
    QUEUE_DISC_MODE_BYTES,               /**< Use number of bytes for maximum queue disc size */
  };


  /**
   * \brief Set the operating mode of this queue disc.
   *
   * \param mode The operating mode of this queue disc.
   */

  void SetMode (QueueDiscMode mode);

  /**
   * \brief Get the operating mode of this queue disc.
   *
   * \returns The operating mode of this queue disc.
   */
  QueueDiscMode GetMode (void);

  /**
   * \brief Get the current value of the queue in bytes or packets.
   *
   * \returns The queue size in bytes or packets.
   */
  uint32_t GetQueueSize (void);
/**
   * \brief Set the limit of the queue.
   *
   * \param lim The limit in bytes or packets.
   */
  void SetQueueLimit (uint32_t lim);

/**Set priority method 0 to leave priority field in header, 1 to use flowid as priority. **/
  void SetPriorityMethod (uint32_t pri);
  /**
   * \brief Set the thresh limits of In and Out RED queue.
   *
   * \param minTh Minimum thresh in bytes or packets.
   * \param maxTh Maximum thresh in bytes or packets.
   */
  void SetTh (double minThIn, double maxThIn,double outMinTh, double outMaxTh);

  /**
   * \brief Get the RIO statistics after running.
   *
   * \returns The drop statistics.
   */
  Stats GetStats ();

  int64_t AssignStreams (int64_t stream);
protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);

  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);

  /**
   * \brief Initialize the queue parameters.
   *
   * Note: if the link bandwidth changes in the course of the
   * simulation, the bandwidth-dependent RIO parameters do not change.
   * This should be fixed, but it would require some extra parameters,
   * and didn't seem worth the trouble...
   */
  virtual void InitializeParams (void);



  /**
   * \brief Compute the average queue size
   * \param nQueued number of queued packets
   * \param m simulated number of packets arrival during idle period
   * \param qAvg average queue size
   * \param qW queue weight given to cur q size sample
   * \returns new average queue size
   */
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);

  /**
   * \brief Check if a IN packet needs to be dropped due to probability mark
   * \param item queue item
   * \param qSize IN queue size
   * \returns 0 for no drop/mark, 1 for drop
   */
  uint32_t DropInEarly (Ptr<QueueDiscItem> item, uint32_t qSize);

  /**
   * \brief Check if a OUT packet needs to be dropped due to probability mark
   * \param item queue item
   * \param qSize total queue size
   * \returns 0 for no drop/mark, 1 for drop
   */
  uint32_t DropOutEarly (Ptr<QueueDiscItem> item, uint32_t qSize);

  /**
   * \brief Returns a probability using these function parameters for the DropEarly function
   * \param qAvg Average queue length
   * \param maxTh Max avg length threshold
   * \param gentle "gentle" algorithm
   * \param vA vA
   * \param vB vB
   * \param vC vC
   * \param vD vD
   * \param maxP max_p
   * \returns Prob. of packet drop before "count"
   */
  double CalculatePNew (double qAvg, double, bool gentle, double vA,
                        double vB, double vC, double vD, double maxP);
  /**
   * \brief Returns a probability using these function parameters for the DropEarly function
   * \param p Prob. of packet drop before "count"
   * \param count number of packets since last random number generation
   * \param countBytes number of bytes since last drop
   * \param meanPktSize Avg pkt size
   * \param wait True for waiting between dropped packets
   * \param size packet size
   * \returns Prob. of packet drop
   */
  double ModifyP (double p, uint32_t count, uint32_t countBytes,
                  uint32_t meanPktSize, bool wait, uint32_t size);

  Stats m_stats; //!< RIO statistics
  // ** Variables supplied by user
  QueueDiscMode m_mode;     //!< Mode (Bytes or packets)
  uint32_t m_meanPktSize;   //!< Avg pkt size
  bool m_isWait;            //!< True for waiting between dropped packets
  bool m_isGentleIn;          //!< True to increases dropping prob. slowly when ave queue exceeds maxthresh
  bool m_isGentleOut;          //!< True to increases dropping prob. slowly when ave queue exceeds maxthresh
  double m_minThIn;           //!< Min avg length threshold (bytes)
  double m_maxThIn;           //!< Max avg length threshold (bytes), should be >= 2*minTh
  double m_minThOut;           //!< Min avg length threshold (bytes)
  double m_maxThOut;           //!< Max avg length threshold (bytes), should be >= 2*minTh
  uint32_t m_queueLimit;    //!< Queue limit in bytes / packets
  double m_qW;              //!< Queue weight given to cur queue size sample
  bool m_isNs1Compat;       //!< Ns-1 compatibility
  DataRate m_linkBandwidth; //!< Link bandwidth
  Time m_linkDelay;         //!< Link delay
  bool m_useEcn;            //!< True if ECN is used (packets are marked instead of being dropped)
  bool m_useHardDrop;       //!< True if packets are always dropped above max threshold
  double m_lIntermIn;         //!< The max probability of dropping a packet
  double m_lIntermOut;         //!< The max probability of dropping a packet


  // ** Variables maintained by RIO
  double m_curMaxPIn;         //!< Current max_p
  double m_curMaxPOut;         //!< Current max_p
  int32_t m_flow;                       //flowid
  double m_vProb1Out;          //!< Prob. of packet drop before "count"
  double m_vAOut;              //!< 1.0 / (m_maxTh - m_minTh)
  double m_vBOut;              //!< -m_minTh / (m_maxTh - m_minTh)
  double m_vCOut;              //!< (1.0 - m_curMaxP) / m_maxTh - used in "gentle" mode
  double m_vDOut;              //!< 2.0 * m_curMaxP - 1.0 - used in "gentle" mode
  double m_vProbOut;           //!< Prob. of packet drop
  uint32_t m_countBytesOut;    //!< Number of bytes since last drop
  uint32_t m_oldOut;           //!< 0 when average queue first exceeds threshold
  bool m_idle;          //!< 0/1 idle status
  double m_vProb1In;          //!< Prob. of packet drop before "count"
  double m_vAIn;              //!< 1.0 / (m_maxTh - m_minTh)
  double m_vBIn;              //!< -m_minTh / (m_maxTh - m_minTh)
  double m_vCIn;              //!< (1.0 - m_curMaxP) / m_maxTh - used in "gentle" mode
  double m_vDIn;              //!< 2.0 * m_curMaxP - 1.0 - used in "gentle" mode
  double m_vProbIn;           //!< Prob. of packet drop
  uint32_t m_countBytesIn;    //!< Number of bytes since last drop
  uint32_t m_countBytes;    //!< Number of bytes since last drop
  uint32_t m_oldIn;           //!< 0 when average queue first exceeds threshold
  bool m_idleIn;          //!< 0/1 idle status
  double m_ptc;             //!< packet time constant in packets/second
  double m_qAvg;            //!< Average queue length
  double m_qAvgIn;            //!< Average queue length
  uint32_t m_count;         //!< Number of packets since last random number generation
  uint32_t m_countOut;         //!< Number of packets since last random number generation
  uint32_t m_countIn;         //!< Number of packets since last random number generation
  Time m_idleTime;          //!< Start of current idle period

  Ptr<UniformRandomVariable> m_uv;  //!< rng stream

  uint32_t inlen;       /* In Packets count */
  uint32_t inbcount;    /* In packets byte count */

  uint32_t m_priorityMethod;    /* 0 to leave priority field in header, */
                                /*  1 to use flowid as priority.  */


};


};  // namespace ns3

#endif // RIO_QUEUE_DISC_H
