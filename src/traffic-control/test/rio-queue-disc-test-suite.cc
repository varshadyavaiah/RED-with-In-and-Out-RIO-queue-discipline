#include "ns3/test.h"
#include "ns3/rio-queue-disc.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

using namespace ns3;

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Rio Queue Disc Test Item
 */
class RioQueueDiscTestItem : public QueueDiscItem {
public:
  /**
   * Constructor
   *
   * \param p packet
   * \param addr address
   * \param protocol protocol
   * \param ecnCapable ECN capable flag
   */
  RioQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, bool ecnCapable);
  virtual ~RioQueueDiscTestItem ();
  virtual void AddHeader (void);
  virtual bool Mark(void);

private:
  RioQueueDiscTestItem ();
  /**
   * \brief Copy constructor
   * Disable default implementation to avoid misuse
   */
  RioQueueDiscTestItem (const RioQueueDiscTestItem &);
  /**
   * \brief Assignment operator
   * \return this object
   * Disable default implementation to avoid misuse
   */
  RioQueueDiscTestItem &operator = (const RioQueueDiscTestItem &);
  bool m_ecnCapablePacket; ///< ECN capable packet?
};

RioQueueDiscTestItem::RioQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, bool ecnCapable)
  : QueueDiscItem (p, addr, protocol),
    m_ecnCapablePacket (ecnCapable)
{
}

RioQueueDiscTestItem::~RioQueueDiscTestItem ()
{
}

void
RioQueueDiscTestItem::AddHeader (void)
{
}

bool
RioQueueDiscTestItem::Mark (void)
{
  if (m_ecnCapablePacket)
    {
      return true;
    }
  return false;
}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Rio Queue Disc Test Case
 */
class RioQueueDiscTestCase : public TestCase
{
public:
  RioQueueDiscTestCase ();
  virtual void DoRun (void);
private:
  /**
   * Enqueue function
   * \param queue the queue disc
   * \param size the size
   * \param nPkt the number of packets
   * \param ecnCapable ECN capable flag
   */
  void Enqueue (Ptr<RioQueueDisc> queue, uint32_t size, uint32_t nPkt, bool ecnCapable);
  /**
   * Run RIO test function
   * \param mode the mode
   */
  void RunRioTest (StringValue mode);
};

RioQueueDiscTestCase::RioQueueDiscTestCase ()
  : TestCase ("Sanity check on the rio queue implementation")
{
}

void
RioQueueDiscTestCase::RunRioTest (StringValue mode)
{
  uint32_t pktSize = 0;
  // 1 for packets; pktSize for bytes
  uint32_t modeSize = 1;
  double minThIn = 6;
  double maxThIn = 12;
  double minThOut = 3;
  double maxThOut = 6;
  uint32_t qSize = 8;
  Ptr<RioQueueDisc> queue = CreateObject<RioQueueDisc> ();

  // test 1: simple enqueue/dequeue with no drops
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");

  Address dest;
  
  if (queue->GetMode () == RioQueueDisc::QUEUE_DISC_MODE_BYTES)
    {
      // pktSize should be same as MeanPktSize to avoid performance gap between byte and packet mode
      pktSize = 500;
      modeSize = pktSize;
      queue->SetTh (minThIn * modeSize, maxThIn * modeSize, minThOut * modeSize, maxThOut * modeSize);
      queue->SetQueueLimit (qSize * modeSize);
    }

  Ptr<Packet> p1, p2, p3, p4, p5, p6, p7, p8;
  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (pktSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);
  p7 = Create<Packet> (pktSize);
  p8 = Create<Packet> (pktSize);

  queue->Initialize ();
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 0 * modeSize, "There should be no packets in there");
  queue->Enqueue (Create<RioQueueDiscTestItem> (p1, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 1 * modeSize, "There should be one packet in there");
  queue->Enqueue (Create<RioQueueDiscTestItem> (p2, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 2 * modeSize, "There should be two packets in there");
  queue->Enqueue (Create<RioQueueDiscTestItem> (p3, dest, 0, false));
  queue->Enqueue (Create<RioQueueDiscTestItem> (p4, dest, 0, false));
  queue->Enqueue (Create<RioQueueDiscTestItem> (p5, dest, 0, false));
  queue->Enqueue (Create<RioQueueDiscTestItem> (p6, dest, 0, false));
  queue->Enqueue (Create<RioQueueDiscTestItem> (p7, dest, 0, false));
  queue->Enqueue (Create<RioQueueDiscTestItem> (p8, dest, 0, false));
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 8 * modeSize, "There should be eight packets in there");

  Ptr<QueueDiscItem> item;

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the first packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 7 * modeSize, "There should be seven packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p1->GetUid (), "was this the first packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the second packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 6 * modeSize, "There should be six packet in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p2->GetUid (), "Was this the second packet ?");

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item != 0), true, "I want to remove the third packet");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (), 5 * modeSize, "There should be five packets in there");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetUid (), p3->GetUid (), "Was this the third packet ?");

  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();
  item = queue->Dequeue ();

  item = queue->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "There are really no packets in there");


  // test 2: more data, but with no drops
  queue = CreateObject<RioQueueDisc> ();
  minThIn = 120 * modeSize;
  maxThIn = 240 * modeSize;
  minThOut = 40 * modeSize;
  maxThOut = 120 * modeSize;
  qSize = 300 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  RioQueueDisc::Stats st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should zero dropped packets due probability mark");
  NS_TEST_EXPECT_MSG_EQ (st.forcedDrop, 0, "There should zero dropped packets due hardmark mark");
  NS_TEST_EXPECT_MSG_EQ (st.qLimDrop, 0, "There should zero dropped packets due queue full");

  // save number of drops from tests
  struct d {
    uint32_t test3;
    uint32_t test4;
    uint32_t test5;
    uint32_t test6;
    uint32_t test7;
  } drop;


  // test 3: more data, now drops due QW change
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  drop.test3 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_NE (drop.test3, 0, "There should be some dropped packets");


  // test 4: riouced maxTh, this causes more drops
  maxThIn = 200 * modeSize;
  maxThOut = 100 * modeSize;
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  drop.test4 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test4, drop.test3, "Test 4 should have more drops than test 3");


  // test 5: change drop probability to a high value (LInterm)
  maxThIn = 300 * modeSize;
  maxThOut = 150 * modeSize;
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (5)), true,
                         "Verify that we can actually set the attribute LInterm");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  drop.test5 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test5, drop.test3, "Test 5 should have more drops than test 3");


  // test 6: disable Gentle param
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
 NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute Gentle");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  drop.test6 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test6, drop.test3, "Test 6 should have more drops than test 3");


  // test 7: disable Wait param
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.020)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Wait", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute Wait");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  drop.test7 = st.unforcedDrop + st.forcedDrop + st.qLimDrop;
  NS_TEST_EXPECT_MSG_GT (drop.test7, drop.test3, "Test 7 should have more drops than test 3");


  // test 8: RIO queue disc is ECN enabled, but packets are not ECN capable
  queue = CreateObject<RioQueueDisc> ();
  minThIn = 90 * modeSize;
  maxThIn = 180 * modeSize;
  minThOut = 30 * modeSize;
  maxThOut = 90 * modeSize;
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, false);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  // Packets are not ECN capable, so there should be only unforced drops, no unforced marks
  NS_TEST_EXPECT_MSG_NE (st.unforcedDrop, 0, "There should be some unforced drops");
  NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced marks");


  // test 9: Packets are ECN capable, but RIO queue disc is not ECN enabled
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (false)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, true);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  // RIO queue disc is not ECN enabled, so there should be only unforced drops, no unforced marks
  NS_TEST_EXPECT_MSG_NE (st.unforcedDrop, 0, "There should be some unforced drops");
  NS_TEST_EXPECT_MSG_EQ (st.unforcedMark, 0, "There should be no unforced marks");


  // test 10: Packets are ECN capable and RIO queue disc is ECN enabled
  queue = CreateObject<RioQueueDisc> ();
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
                         "Verify that we can actually set the attribute Mode");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThIn", DoubleValue (minThIn)), true,
                         "Verify that we can actually set the attribute MinThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThIn", DoubleValue (maxThIn)), true,
                         "Verify that we can actually set the attribute MaxThIn");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MinThOut", DoubleValue (minThOut)), true,
                         "Verify that we can actually set the attribute MinThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("MaxThOut", DoubleValue (maxThOut)), true,
                         "Verify that we can actually set the attribute MaxThOut");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QueueLimit", UintegerValue (qSize)), true,
                         "Verify that we can actually set the attribute QueueLimit");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("QW", DoubleValue (0.002)), true,
                         "Verify that we can actually set the attribute QW");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("LInterm", DoubleValue (2)), true,
                         "Verify that we can actually set the attribute LInterm");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Gentle", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute Gentle");
  NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("UseEcn", BooleanValue (true)), true,
                         "Verify that we can actually set the attribute UseECN");
  queue->Initialize ();
  Enqueue (queue, pktSize, 300, true);
  st = StaticCast<RioQueueDisc> (queue)->GetStats ();
  // Packets are ECN capable, RIO queue disc is ECN enabled; there should be only unforced marks, no unforced drops
  NS_TEST_EXPECT_MSG_EQ (st.unforcedDrop, 0, "There should be no unforced drops");
  NS_TEST_EXPECT_MSG_NE (st.unforcedMark, 0, "There should be some unforced marks");
}

void 
RioQueueDiscTestCase::Enqueue (Ptr<RioQueueDisc> queue, uint32_t size, uint32_t nPkt, bool ecnCapable)
{
  Address dest;
  for (uint32_t i = 0; i < nPkt; i++)
    {
      queue->Enqueue (Create<RioQueueDiscTestItem> (Create<Packet> (size), dest, 0, ecnCapable));
    }
}

void
RioQueueDiscTestCase::DoRun (void)
{
  RunRioTest (StringValue ("QUEUE_DISC_MODE_PACKETS"));
  RunRioTest (StringValue ("QUEUE_DISC_MODE_BYTES"));
  Simulator::Destroy ();

}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Rio Queue Disc Test Suite
 */
static class RioQueueDiscTestSuite : public TestSuite
{
public:
  RioQueueDiscTestSuite ()
    : TestSuite ("rio-queue-disc", UNIT)
  {
    AddTestCase (new RioQueueDiscTestCase (), TestCase::QUICK);
  }
} g_rioQueueTestSuite; ///< the test suite
