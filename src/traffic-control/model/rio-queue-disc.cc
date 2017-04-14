#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "rio-queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/net-device-queue-interface.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RioQueueDisc");
NS_OBJECT_ENSURE_REGISTERED (RioQueueDisc);

TypeId RioQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RioQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<RioQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (QUEUE_DISC_MODE_PACKETS),
                   MakeEnumAccessor (&RioQueueDisc::SetMode),
                   MakeEnumChecker (QUEUE_DISC_MODE_BYTES, "QUEUE_DISC_MODE_BYTES",
                                    QUEUE_DISC_MODE_PACKETS, "QUEUE_DISC_MODE_PACKETS"))
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (500),
                   MakeUintegerAccessor (&RioQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("IdlePktSize",
                   "Average packet size used during idle times. Used when m_cautions = 3",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RioQueueDisc::m_idlePktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Wait",
                   "True for waiting between dropped packets",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RioQueueDisc::m_isWait),
                   MakeBooleanChecker ())
    .AddAttribute ("GentleIn",
                   "True to increases dropping probability slowly when average queue exceeds maxthresh",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RioQueueDisc::m_isGentleIn),
                   MakeBooleanChecker ())
    .AddAttribute ("GentleOut",
                   "True to increases dropping probability slowly when average queue exceeds maxthresh",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RioQueueDisc::m_isGentleOut),
                   MakeBooleanChecker ())
    .AddAttribute ("MinThIn",
                   "Minimum average length threshold of In queue in packets/bytes",
                   DoubleValue (15),
                   MakeDoubleAccessor (&RioQueueDisc::m_minThIn),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinThOut",
                   "Minimum average length threshold of Out queue in packets/bytes",
                   DoubleValue (5),
                   MakeDoubleAccessor (&RioQueueDisc::m_minThOut),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxThIn",
                   "Maximum average length threshold of In queue in packets/bytes",
                   DoubleValue (30),
                   MakeDoubleAccessor (&RioQueueDisc::m_maxThIn),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MaxThOut",
                   "Maximum average length threshold of Out queue in packets/bytes",
                   DoubleValue (15),
                   MakeDoubleAccessor (&RioQueueDisc::m_maxThOut),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (25),
                   MakeUintegerAccessor (&RioQueueDisc::SetQueueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QW",
                   "Queue weight related to the exponential weighted moving average (EWMA)",
                   DoubleValue (0.002),
                   MakeDoubleAccessor (&RioQueueDisc::m_qW),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("LIntermIn",
                   "The maximum probability of dropping a IN packet",
                   DoubleValue (50),
                   MakeDoubleAccessor (&RioQueueDisc::m_lIntermIn),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("LIntermOut",
                   "The maximum probability of dropping a OUT packet",
                   DoubleValue (50),
                   MakeDoubleAccessor (&RioQueueDisc::m_lIntermOut),
                   MakeDoubleChecker <double> ())
    .AddAttribute ("Ns1Compat",
                   "NS-1 compatibility",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RioQueueDisc::m_isNs1Compat),
                   MakeBooleanChecker ())
    .AddAttribute ("LinkBandwidth",
                   "The RIO link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&RioQueueDisc::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("LinkDelay",
                   "The RIO link delay",
                   TimeValue (MilliSeconds (20)),
                   MakeTimeAccessor (&RioQueueDisc::m_linkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("UseEcn",
                   "True to use ECN (packets are marked instead of being dropped)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RioQueueDisc::m_useEcn),
                   MakeBooleanChecker ())
    .AddAttribute ("UseHardDrop",
                   "True to always drop packets above max threshold",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RioQueueDisc::m_useHardDrop),
                   MakeBooleanChecker ())
    .AddAttribute ("PriorityMethod",
                   "0 to leave priority field in header, 1 to use flowid as priority.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&RioQueueDisc::SetPriorityMethod),
                   MakeUintegerChecker<uint32_t> ())
  ;

  return tid;
}

RioQueueDisc::RioQueueDisc ()
  : QueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

RioQueueDisc::~RioQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
RioQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
}

void
RioQueueDisc::SetMode (QueueDiscMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

RioQueueDisc::QueueDiscMode
RioQueueDisc::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

void
RioQueueDisc::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_queueLimit = lim;
}

void
RioQueueDisc::SetPriorityMethod (uint32_t pri)
{
  NS_LOG_FUNCTION (this << pri);
  m_priorityMethod = pri;
}

void
RioQueueDisc::SetTh (double minThIn, double maxThIn,double minThOut, double maxThOut)
{
  NS_LOG_FUNCTION (this << minThIn << maxThIn << minThOut << maxThOut);
  NS_ASSERT (minThIn <= maxThIn);
  NS_ASSERT (minThOut <= maxThOut);
  m_minThIn = minThIn;
  m_maxThIn = maxThIn;
  m_minThOut = minThOut;
  m_maxThOut = maxThOut;
}

RioQueueDisc::Stats
RioQueueDisc::GetStats ()
{
  NS_LOG_FUNCTION (this);
  return m_stats;
}


int64_t
RioQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}


/* variables added m_idleIn, m_countBytesIn, nQueuedIn */

Ptr<QueueDiscItem>
RioQueueDisc::DoDequeue (void)
{
  Ptr<QueueDiscItem> p;

  NS_LOG_FUNCTION (this);

  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      m_idle = 1;
      m_idleTime = Simulator::Now ();

      p=0;
    }
  else
    {
      m_idle = 0;
      Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();

      NS_LOG_LOGIC ("Popped " << item);

      NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
      NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

      p=item;
    }

  if (p != 0)
    {
     
      m_flow = Classify(p);

      if (m_flow)
        {
          /* Regular In packets */
          m_idleIn = 0;
          inbcount -= p->GetSize ();
          --inlen;
        }
    }
  else
    {
      m_idleIn = 1;
    }
  return (p);
}


double
RioQueueDisc::Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW)
{
  NS_LOG_FUNCTION (this << nQueued << m << qAvg << qW);

  double newAve = qAvg * pow (1.0 - qW, m);
  newAve += qW * nQueued;

  return newAve;
}

uint32_t
RioQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == QUEUE_DISC_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode () == QUEUE_DISC_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown RIO mode.");
    }
}

/*
 * Receive a new packet arriving at the queue.
 * The average queue size is computed.  If the average size
 * exceeds the threshold, then the dropping probability is computed,
 * and the newly-arriving packet is dropped with that probability.
 * The packet is also dropped if the maximum queue size is exceeded.
 *
 * "Forced" drops mean a packet arrived when the underlying queue was
 * full or when the average q size exceeded maxthresh.
 * "Unforced" means a RED random drop.
 *
 * For forced drops, either the arriving packet is dropped or one in the
 * queue is dropped, depending on the setting of drop_tail_.
 * For unforced drops, the arriving packet is always the victim.
 */



/* Variables used m_priorityMethod, m_inidle,m_idle,m_idletime,m_qavg,m_inqavg,m_incount,m_incountbytes,m_outminth,m_outmaxth,m_outisgentle,moutold,moutcount,moutcountbytes */
bool RioQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  /* Duplicate the RED algorithm to carry out a separate
   * calculation for Out packets -- Wenjia */
  /*Add ns3 equivalent */

  if (m_priorityMethod == 1)
    {
      /*Add ns3 equivalent */
      m_flow = Classify (item);
    }

  uint32_t qLen;
  uint32_t qLenIn;

  if (GetMode () == QUEUE_DISC_MODE_BYTES)
    {

      qLen = GetInternalQueue (0)->GetNBytes ();
      qLenIn = inbcount;
    }
  else
    {
      qLen = GetInternalQueue (0)->GetNPackets ();
      qLenIn = inlen;
    }
  /*Add ns3 equivalent */

  uint32_t m = 0;
  Time now = Simulator::Now ();
  if (m_idle)
    {
      m_idle = 0;
      m = uint32_t (m_ptc * (now - m_idleTime).GetSeconds ());
    }
  m_qAvg = Estimator (qLen,m + 1,m_qAvg, m_qW);

  if (m_flow)      /* Regular In packets */

    {
      /*
       * if we were idle, we pretend that m packets arrived during
       * the idle period.  m is set to be the ptc times the amount
       * of time we've been idle for
       */


      uint32_t m_in = 0;

      /* To account for the period when the queue was empty.  */
      if (m_idleIn)
        {
          m_idleIn = 0;
          m_in = uint32_t (m_ptc * (now - m_idleTime).GetSeconds ());
        }


      /*
       * Run the estimator with either 1 new packet arrival, or with
       * the scaled version above [scaled by m due to idle time]
       */

      // printf( "qlen %d\n", q_->length());



      m_qAvgIn = Estimator (qLenIn,m_in + 1,m_qAvgIn, m_qW);

      /*
       * count and count_bytes keeps a tally of arriving traffic
       * that has not been dropped (i.e. how long, in terms of traffic,
       * it has been since the last early drop)
       */

      ++m_count;
      m_countBytes += item->GetSize ();

      ++m_countIn;
      m_countBytesIn += item->GetSize ();

      /*
             * DROP LOGIC:
             *    q = current q size, ~q = averaged q size
             *    1> if ~q > maxthresh, this is a FORCED drop
             *    2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
             *    3> if (q+1) > hard q limit, this is a FORCED drop
             */

      /* if the average queue is below the out_th_min
       * then we have no need to worry.
       */


      // register double in_qavg = edv_in_.v_ave;
      uint32_t dropType = DTYPE_NONE;


      if (m_qAvgIn >= m_minThIn && qLenIn > 1)
        {
          if ((!m_isGentleIn && m_qAvgIn >= m_maxThIn)
              || (m_isGentleIn && m_qAvgIn >= 2 * m_maxThIn))
            {
              dropType = DTYPE_FORCED;            //
            }
          else if (m_oldIn == 0)
            {
/*
                         * The average queue size has just crossed the
                         * threshold from below to above "minthresh", or
                         * from above "minthresh" with an empty queue to
                         * above "minthresh" with a nonempty queue.
                         */
              m_countIn = 1;
              m_countBytesIn = item->GetSize ();
              m_oldIn = 1;
            }
          else if (DropInEarly (item,qLen))
            {
              dropType = DTYPE_UNFORCED;           // ? not sure, Yun
            }
        }
      else
        {
          m_vProbIn = 0.0;
          m_oldIn = 0;                      // explain
        }
      if (qLen >= m_queueLimit)
        {
          // see if we've exceeded the queue size
          dropType = DTYPE_FORCED;           //need more consideration, Yun
        }

      if (dropType == DTYPE_UNFORCED)       //how does ecn work?
        {
          if (!m_useEcn || !item->Mark ())
            {
              NS_LOG_DEBUG ("\t Dropping In pkt due to Prob Mark " << m_qAvgIn);
              m_stats.unforcedDrop++;
              --inlen;
              inbcount -= item->GetSize ();
              Drop (item);

              return false;
            }
          NS_LOG_DEBUG ("\t Marking In pkt due to Prob Mark " << m_qAvgIn); // where are we marking?
          m_stats.unforcedMark++;

        }

      else if (dropType == DTYPE_FORCED)
        {
          if (m_useHardDrop || !m_useEcn || !item->Mark ())
            {
              NS_LOG_DEBUG ("\t Dropping In pkt due to Hard Mark " << m_qAvgIn);
              m_stats.forcedDrop++;
              --inlen;
              inbcount -= item->GetSize ();
              Drop (item);
              if (m_isNs1Compat)
                {
                  m_count = 0;
                  m_countBytes = 0;
                  m_countIn = 0;      //added by me, review it
                  m_countBytesIn = 0;      //added by me,review it
                }
              return false;
            }
          NS_LOG_DEBUG ("\t Marking In pkt due to Hard Mark " << m_qAvgIn);
          m_stats.forcedMark++;

        }
      bool retval = GetInternalQueue (0)->Enqueue (item);
      ++inlen;
      inbcount += item->GetSize ();
      if (!retval)
        {
          m_stats.qLimDrop++;
        }

      // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
      // because QueueDisc::AddInternalQueue sets the drop callback

      NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
      NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

      return retval;
    }
  else
    {
      //out pkts
/* Out packets and default regular packets */
      /*
       * count and count_bytes keeps a tally of arriving traffic
       * that has not been dropped (i.e. how long, in terms of traffic,
       * it has been since the last early drop)
       */

      ++m_count;
      m_countBytes += item->GetSize ();

      /* added by Yun */
      ++m_countOut;
      m_countBytesOut += item->GetSize ();

      /*
* DROP LOGIC:
*    q = current q size, ~q = averaged q size
*    1> if ~q > maxthresh, this is a FORCED drop
*    2> if minthresh < ~q < maxthresh, this may be an UNFORCED drop
*    3> if (q+1) > hard q limit, this is a FORCED drop
*/

      /* if the average queue is below the out_th_min
       * then we have no need to worry.
       */


      // register double in_qavg = edv_in_.v_ave;
      int dropType = DTYPE_NONE;
      //int qlen = qib_ ? q_->byteLength() : q_->length();
      /* added by Yun, seems not useful */
      // int out_qlen = qib_ ? out_bcount_ : q_->out_length();

      //int qlim = qib_ ?  (qlim_ * edp_.mean_pktsize) : qlim_;

      //curq_ = qlen; // helps to trace queue during arrival, if enabled

      if (m_qAvg >= m_minThOut && qLen > 1)
        {
          if ((!m_isGentleOut && m_qAvg >= m_maxThOut)
              || (m_isGentleOut && m_qAvg >= 2 * m_maxThOut))
            {
              dropType = DTYPE_FORCED;            // ? not sure, Yun
            }
          else if (m_oldOut == 0)
            {

              /*
               * The average queue size has just crossed the
               * threshold from below to above "minthresh", or
               * from above "minthresh" with an empty queue to
               * above "minthresh" with a nonempty queue.
               */
              m_countOut = 1;
              m_countBytesOut = item->GetSize ();
              m_oldOut = 1;
            }
          else if (DropOutEarly (item,qLen))
            {
              dropType = DTYPE_UNFORCED;           // ? not sure, Yun
            }
        }
      else
        {
          m_vProbOut = 0.0;
          m_oldOut = 0;                      // explain
        }
      if (qLen >= m_queueLimit)
        {
          // see if we've exceeded the queue size
          dropType = DTYPE_FORCED;           //need more consideration, Yun
        }

      if (dropType == DTYPE_UNFORCED)
        {
          if (!m_useEcn || !item->Mark ())
            {
              NS_LOG_DEBUG ("\t Dropping Out pkt due to Prob Mark " << m_qAvg);
              m_stats.unforcedDrop++;
              Drop (item);
              return false;
            }
          NS_LOG_DEBUG ("\t Marking Out pkt due to Prob Mark " << m_qAvg);
          m_stats.unforcedMark++;


        }

      else if (dropType == DTYPE_FORCED)
        {
          if (m_useHardDrop || !m_useEcn || !item->Mark ())
            {
              NS_LOG_DEBUG ("\t Dropping Out pkt due to Hard Mark " << m_qAvg);
              m_stats.forcedDrop++;
              Drop (item);
              if (m_isNs1Compat)
                {
                  m_countOut = 0;      //added by me, review it
                  m_countBytesOut = 0;      //added by me,review it
                }
              return false;
            }
          NS_LOG_DEBUG ("\t Marking Out pkt due to Hard Mark " << m_qAvg);
          m_stats.forcedMark++;
        }
      bool retval = GetInternalQueue (0)->Enqueue (item);

      if (!retval)
        {
          m_stats.qLimDrop++;
        }

      // If Queue::Enqueue fails, QueueDisc::Drop is called by the internal queue
      // because QueueDisc::AddInternalQueue sets the drop callback

      NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
      NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

      return retval;

    }

}


// Returns a probability using these function parameters for the DropEarly funtion
double
RioQueueDisc::CalculatePNew (double qAvg, double maxTh, bool isGentle, double vA,
                             double vB, double vC, double vD, double maxP)
{
  NS_LOG_FUNCTION (this << qAvg << maxTh << isGentle << vA << vB << vC << vD << maxP);
  double p;

  if (isGentle && qAvg >= maxTh)
    {
      // p ranges from maxP to 1 as the average queue
      // Size ranges from maxTh to twice maxTh
      p = vC * qAvg + vD;
    }
  else if (!isGentle && qAvg >= maxTh)
    {
      /*
       * OLD: p continues to range linearly above max_p as
       * the average queue size ranges above th_max.
       * NEW: p is set to 1.0
       */
      p = 1.0;
    }
  else
    {
      /*
       * p ranges from 0 to max_p as the average queue size ranges from
       * th_min to th_max
       */
      p = vA * qAvg + vB;
      p *= maxP;
    }

  if (p > 1.0)
    {
      p = 1.0;
    }

  return p;
}
// Returns a probability using these function parameters for the DropEarly funtion
double
RioQueueDisc::ModifyP (double p, uint32_t count, uint32_t countBytes,
                       uint32_t meanPktSize, bool isWait, uint32_t size)
{
  NS_LOG_FUNCTION (this << p << count << countBytes << meanPktSize << isWait << size);
  double count1 = (double) count;

  if (GetMode () == QUEUE_DISC_MODE_BYTES)
    {
      count1 = (double) (countBytes / meanPktSize);
    }

  if (isWait)
    {
      if (count1 * p < 1.0)
        {
          p = 0.0;
        }
      else if (count1 * p < 2.0)
        {
          p /= (2.0 - count1 * p);
        }
      else
        {
          p = 1.0;
        }
    }
  else
    {
      if (count1 * p < 1.0)
        {
          p /= (1.0 - count1 * p);
        }
      else
        {
          p = 1.0;
        }
    }

  if ((GetMode () == QUEUE_DISC_MODE_BYTES) && (p < 1.0))
    {
      p = (p * size) / meanPktSize;
    }

  if (p > 1.0)
    {
      p = 1.0;
    }

  return p;
}



/*
 * should the packet be dropped/marked due to a probabilistic drop?
 */
uint32_t
RioQueueDisc::DropInEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
  NS_LOG_FUNCTION (this << item << qSize);

  m_vProb1In = CalculatePNew (m_qAvgIn, m_maxThIn,
                              m_isGentleIn, m_vAIn, m_vBIn, m_vCIn,
                              m_vDIn, m_curMaxPIn);
  m_vProbIn = ModifyP (m_vProb1In, m_countIn,
                       m_countBytesIn, m_meanPktSize, m_isWait,
                       item->GetSize ());

  // drop probability is computed, pick random number and act
  double u = m_uv->GetValue ();
  if (u <= m_vProbIn)
    {
      NS_LOG_LOGIC ("u <= m_vProbIn; u " << u << "; m_vProbIn " << m_vProbIn);

      // DROP or MARK
      m_countIn = 0;
      m_countBytesIn = 0;
      /// \todo Implement set bit to mark

      return 1; // drop
    }
  return (0);                           // no DROP/mark
}


uint32_t
RioQueueDisc::DropOutEarly (Ptr<QueueDiscItem> item, uint32_t qSize)
{
  NS_LOG_FUNCTION (this << item << qSize);

  m_vProb1Out = CalculatePNew (m_qAvg, m_maxThOut,
                               m_isGentleOut, m_vAOut, m_vBOut, m_vCOut,
                               m_vDOut, m_curMaxPOut);
  m_vProbOut = ModifyP (m_vProb1Out, m_countOut,
                        m_countBytesOut, m_meanPktSize, m_isWait,
                        item->GetSize ());


  // drop probability is computed, pick random number and act
  double u = m_uv->GetValue ();
  if (u <= m_vProbOut)
    {
      NS_LOG_LOGIC ("u <= m_vProbOut; u " << u << "; m_vProbOut " << m_vProbOut);

      // DROP or MARK
      m_countOut = 0;
      m_countBytesOut = 0;
      /// \todo Implement set bit to mark

      return 1; // drop
    }
  else
    {
      return (1);
    }

  return (0);        // no DROP/mark
}


Ptr<const QueueDiscItem>
RioQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}


bool
RioQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("RioQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("RioQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<InternalQueue> queue = CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> > ("Mode", EnumValue (m_mode));
      if (m_mode == QUEUE_DISC_MODE_PACKETS)
        {
          queue->SetMaxPackets (m_queueLimit);
        }
      else
        {
          queue->SetMaxBytes (m_queueLimit);
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("RioQueueDisc needs 1 internal queue");
      return false;
    }

  if ((GetInternalQueue (0)->GetMode () == QueueBase::QUEUE_MODE_PACKETS && m_mode == QUEUE_DISC_MODE_BYTES)
      || (GetInternalQueue (0)->GetMode () == QueueBase::QUEUE_MODE_BYTES && m_mode == QUEUE_DISC_MODE_PACKETS))
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the RioQueueDisc");
      return false;
    }

  if ((m_mode ==  QUEUE_DISC_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () < m_queueLimit)
      || (m_mode ==  QUEUE_DISC_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () < m_queueLimit))
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

void
RioQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Initializing RIO params.");

  m_ptc = m_linkBandwidth.GetBitRate () / (8.0 * m_meanPktSize);

  if (m_minThIn == 0 && m_maxThIn == 0)
    {
      m_minThIn = 15.0;

      if (GetMode () == QUEUE_DISC_MODE_BYTES)
        {
          m_minThIn = m_minThIn * m_meanPktSize;
        }

      // default values taken from ns2
      m_maxThIn = 30.0;
    }

  if (m_minThOut == 0 && m_maxThOut == 0)
    {
      m_minThOut = 5.0;

      if (GetMode () == QUEUE_DISC_MODE_BYTES)
        {
          m_minThOut = m_minThOut * m_meanPktSize;
        }

      // why don't they do * m_meanpktsize?
      m_maxThOut = 15.0;
    }
  NS_ASSERT (m_minThIn <= m_maxThIn);
  NS_ASSERT (m_minThOut <= m_maxThOut);
  m_stats.forcedDrop = 0;
  m_stats.unforcedDrop = 0;
  m_stats.qLimDrop = 0;
  m_stats.forcedMark = 0;
  m_stats.unforcedMark = 0;

  m_qAvg = 0.0;
  m_qAvgIn = 0.0;
  m_countOut = 0;
  m_countIn = 0;
  m_countBytesOut = 0;
  m_countBytesIn = 0;
  m_oldOut = 0;
  m_oldIn = 0;
  m_idle = 1;
  m_idleIn = 1;

  inlen = 0;
  inbcount = 0;

  m_priorityMethod = 1;

  double th_diffIn = (m_maxThIn - m_minThIn);
  if (th_diffIn == 0)
    {
      th_diffIn = 1.0;
    }
  m_vAIn = 1.0 / th_diffIn;
  m_curMaxPIn = 1.0 / m_lIntermIn;
  m_vBIn = -m_minThIn / th_diffIn;

  if (m_isGentleIn)
    {
      m_vCIn = (1.0 - m_curMaxPIn) / m_maxThIn;
      m_vDIn = 2.0 * m_curMaxPIn - 1.0;
    }

  double th_diffOut = (m_maxThOut - m_minThOut);
  if (th_diffOut == 0)
    {
      th_diffOut = 1.0;
    }
  m_vAOut = 1.0 / th_diffOut;
  m_curMaxPOut = 1.0 / m_lIntermOut;
  m_vBOut = -m_minThOut / th_diffOut;

  if (m_isGentleOut)
    {
      m_vCOut = (1.0 - m_curMaxPOut) / m_maxThOut;
      m_vDOut = 2.0 * m_curMaxPOut - 1.0;
    }

  m_idleTime = NanoSeconds (0);

/*
 * If m_qW=0, set it to a reasonable value of 1-exp(-1/C)
 * This corresponds to choosing m_qW to be of that value for
 * which the packet time constant -1/ln(1-m)qW) per default RTT
 * of 100ms is an order of magnitude more than the link capacity, C.
 *
 * If m_qW=-1, then the queue weight is set to be a function of
 * the bandwidth and the link propagation delay.  In particular,
 * the default RTT is assumed to be three times the link delay and
 * transmission delay, if this gives a default RTT greater than 100 ms.
 *
 * If m_qW=-2, set it to a reasonable value of 1-exp(-10/C).
 */
  if (m_qW == 0.0)
    {
      m_qW = 1.0 - std::exp (-1.0 / m_ptc);
    }
  else if (m_qW == -1.0)
    {
      double rtt = 3.0 * (m_linkDelay.GetSeconds () + 1.0 / m_ptc);

      if (rtt < 0.1)
        {
          rtt = 0.1;
        }
      m_qW = 1.0 - std::exp (-1.0 / (10 * rtt * m_ptc));
    }
  else if (m_qW == -2.0)
    {
      m_qW = 1.0 - std::exp (-10.0 / m_ptc);
    }

  NS_LOG_DEBUG ("\tm_delay " << m_linkDelay.GetSeconds () << "; m_isWait "
                             << m_isWait << "; m_qW " << m_qW << "; m_ptc " << m_ptc
                             << "; m_minThIn " << m_minThIn << "; m_maxThIn " << m_maxThIn
                             << "; m_minThOut " << m_minThOut << "; m_maxThOut " << m_maxThOut
                             << "; m_isGentleIn " << m_isGentleIn << "; m_isGentleOut " << m_isGentleOut
                             << "; th_diffIn " << th_diffIn << "; th_diffOut " << th_diffOut
                             << "; lIntermIn " << m_lIntermIn << "; lIntermOut " << m_lIntermOut
                             << "; vaIn " << m_vAIn << "; vaOut " << m_vAOut
                             <<  "; cur_max_pIn " << m_curMaxPIn <<  "; cur_max_pOut " << m_curMaxPOut
                             << "; v_bIn " << m_vBIn << "; v_bOut " << m_vBOut
                             <<  "; m_vCIn " << m_vCIn <<  "; m_vCOut " << m_vCOut
                             << "; m_vDIn " <<  m_vDIn << "; m_vDOut " <<  m_vDOut);
}
} // namespace ns3
