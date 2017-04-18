.. include:: replace.txt
.. highlight:: cpp

RIO queue disc
---------------------

Random Early Detection In or Out (RIO), is a variant of RED, which is a 
queue discipline that aims to provide early signals to transport protocol 
congestion control. RIO uses the same mechanism as in RED, but is configured 
with two different sets of parameters. When a packet flows in, it is 
tagged as IN or OUT based on service profiles. Depending on if it is  IN or OUT, 
the router calculates the corresponding parameters. This prevents a bunch of 
tail drop-losses that could be because of TCP timeout, because they backoff gracefully,
thus preventing, or pro-longing the advent of congestion. 

Further, having a twin RED implementation, with different parameters, can ensure 
a certain amount of minimum assured capacity. This is because whenever there is congestion, 
the IN packets will be favoured over the OUT packets. Thus, the OUT packets
will be dropped first to reduce congestion.



Model Description
*****************

The source code for the RIO model is located in the directory ``src/traffic-control/model``
and consists of 2 files `rio-queue-disc.h` and `rio-queue-disc.cc` defining a RioQueueDisc
class.   

* class :cpp:class:`RioQueueDisc`: This is the class that implements the main RIO algorithm:

  * ``RioQueueDisc::DoEnqueue ()``: This routine checks whether the  queue is full, and if so, drops the packets and records the number of drops due to queue overflow. The dropping is done based on the IN and OUT tags. ``RioQueueDisc::InOrOut`` is called to get this information. If the queue is not full, it calls  ``RioQueueDisc::DropInEarly()`` or ``RioQueueDisc::DropOutEarly()``, and depending on the value returned, the incoming packet is either enqueued or dropped.
  
  * ``RioQueueDisc::InOrOut ()``: This routine checks if the packet that comes in is an IN or and OUT packet and depending on that, returns true or false. The check is done and is classified based on the DSCP value in the IP header.

  * ``RioQueueDisc::DropInEarly ()``: The decision to enqueue or drop an IN the packet is taken by invoking this routine, which returns an integer value; 0 indicates enqueue and 1 indicates drop.

  * ``RioQueueDisc::DropOutEarly ()``: The decision to enqueue or drop an OUT the packet is taken by invoking this routine, which returns an integer value; 0 indicates enqueue and 1 indicates drop.


  * ``RioQueueDisc::CalculateP ()``: This routine is called to newly calculate the drop probability, which is required by ``RioQueueDisc::DropInEarly()`` and ``RioQueueDisc::DropOutEarly()``. The decision to randomly drop, before congestion builds, is taken based on this value.

  * ``RioQueueDisc::ModifyP ()``: This routine is called to update the drop probability, which is required by ``RioQueueDisc::DropInEarly()`` and ``RioQueueDisc::DropOutEarly()``. The decision to randomly drop, before congestion builds, is taken based on this value.

  * ``RioQueueDisc::DoDequeue ()``: This routine returns a pointer to the packet that is being dequeued.
  
  * ``RioQueueDisc::Estimator()``: The calculates the average queue size, both for IN packets and the full queue. The value returned is used by ``RioQueueDisc::CalculateP ()`` and used to take a decision on the dropping of the packets.
 
 
Explicit Congestion Notification (ECN)
======================================
This RIO model supports an ECN mode of operation to notify endpoints of
congestion that may be developing in a bottleneck queue, without resorting
to packet drops. Such a mode is enabled by setting the UseEcn attribute to
true (it is false by default) and only affects incoming packets with the
ECT bit set in their header. When the average queue length is between the
minimum and maximum thresholds, an incoming packet is marked instead of being
dropped. When the average queue length is above the maximum threshold, an
incoming packet is marked (instead of being dropped) only if the UseHardDrop
attribute is set to false (it is true by default).

The implementation of support for ECN marking is done in such a way as
to not impose an internet module dependency on the traffic control module.
The RIO model does not directly set ECN bits on the header, but delegates
that job to the QueueDiscItem class.  As a result, it is possible to
use RIO queues for other non-IP QueueDiscItems that may or may not support
the ``Mark ()`` method.

References
==========

The RIO implementation is based on the the following:
D.D.Clark, W.Fang http://ieeexplore.ieee.org/abstract/document/720870/

The RED queue used aims to be close to the results cited in:
S.Floyd, K.Fall http://icir.org/floyd/papers/redsims.ps

The addition of explicit congestion notification (ECN) to IP:
K. K. Ramakrishnan et al, https://tools.ietf.org/html/rfc3168

Attributes
==========

The RIO queue contains a number of attributes that control the RIO
policies:

* Mode (bytes or packets)
* MeanPktSize
* IdlePktSize
* Wait (time)
* GentleIn, GentleOut modes
* MinThIn, MinThOut
* MaxThIn, MaxThOut
* QueueLimit
* Queue weight
* LIntermIn, LIntermOut
* LinkBandwidth
* LinkDelay
* UseEcn
* UseHardDrop
* PriorityMethod

Consult the ns-3 documentation for explanation of these attributes.


Examples
========

The RIO queue example is found at ``src/traffic-control/examples/rio-example.cc``.

Validation
**********

The RIO model is tested using :cpp:class:`RioQueueDiscTestSuite` class defined in `src/traffic-control/test/rio-queue-test-suite.cc`. The suite includes 4 test cases:

* Test 1: simple enqueue/dequeue with defaults, no drops
* Test 2: more OUT packet drops than IN packet drops
* Test 3: reducing maxTh, thus increasing the number of drops
* Test 4: increasing the drop probability 

The test suite can be run using the following commands: 

::

  $ ./waf configure --enable-examples --enable-tests
  $ ./waf build
  $ ./test.py -s rio-queue-disc
 
