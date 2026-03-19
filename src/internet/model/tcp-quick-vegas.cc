 #include "tcp-quick-vegas.h"

 #include "tcp-socket-state.h"
 
 #include "ns3/log.h"
 
 namespace ns3
 {
 
 NS_LOG_COMPONENT_DEFINE("TcpQuickVegas");
 NS_OBJECT_ENSURE_REGISTERED(TcpQuickVegas);

 TypeId
 TcpQuickVegas::GetTypeId()
 {
     static TypeId tid = TypeId("ns3::TcpQuickVegas")
                             .SetParent<TcpVegas>()
                             .AddConstructor<TcpQuickVegas>()
                             .SetGroupName("Internet");
                             
     return tid;
 }

 
//  TypeId
//  TcpQuickVegas::GetTypeId()
//  {
//      static TypeId tid = TypeId("ns3::TcpQuickVegas")
//                              .SetParent<TcpVegas>()
//                              .AddConstructor<TcpQuickVegas>()
//                              .SetGroupName("Internet")
//                              .AddAttribute("Alpha",
//                                            "Lower bound of packets in network",
//                                            UintegerValue(1),
//                                            MakeUintegerAccessor(&TcpQuickVegas::m_alpha),
//                                            MakeUintegerChecker<uint32_t>())
//                              .AddAttribute("Beta",
//                                            "Upper bound of packets in network",
//                                            UintegerValue(4),
//                                            MakeUintegerAccessor(&TcpQuickVegas::m_beta),
//                                            MakeUintegerChecker<uint32_t>())
//                              .AddAttribute("Gamma",
//                                            "Limit on increase",
//                                            UintegerValue(2),
//                                            MakeUintegerAccessor(&TcpQuickVegas::m_gamma),
//                                            MakeUintegerChecker<uint32_t>());
//      return tid;
//  }
 
 TcpQuickVegas::TcpQuickVegas()
     : TcpVegas(),
       m_alpha(2),
       m_beta(4),
       m_gamma(1),
       m_baseRtt(Time::Max()),
       m_minRtt(Time::Max()),
       m_cntRtt(0),
       m_doingVegasNow(true),
       m_begSndNxt(0),
       m_succ(0),
       m_incr(0),
       m_rttRounds(0),
       m_status(0),
       m_lastIncr(0),
       m_incr_s(0)
 {
     NS_LOG_FUNCTION(this);
 }
 
 TcpQuickVegas::TcpQuickVegas(const TcpQuickVegas& sock)
     : TcpVegas(sock),
       m_alpha(sock.m_alpha),
       m_beta(sock.m_beta),
       m_gamma(sock.m_gamma),
       m_baseRtt(sock.m_baseRtt),
       m_minRtt(sock.m_minRtt),
       m_cntRtt(sock.m_cntRtt),
       m_doingVegasNow(true), //m_doingVegasNow(sock.m_doingVegasNow) ?
       m_begSndNxt(0),
       m_succ(sock.m_succ),
       m_incr(sock.m_incr),
       m_rttRounds(sock.m_rttRounds),
       m_status(sock.m_status),       
       m_lastIncr(sock.m_lastIncr),  
       m_incr_s(sock.m_incr_s)        
 {
     NS_LOG_FUNCTION(this);
 }
 
 TcpQuickVegas::~TcpQuickVegas()
 {
     NS_LOG_FUNCTION(this);
 }
 
 Ptr<TcpCongestionOps>
 TcpQuickVegas::Fork()
 {
     return CopyObject<TcpQuickVegas>(this);
 }
 


void
TcpQuickVegas::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{

    if (rtt.IsZero())
     {
         return;
     }


    // Check for RTT improvement against m_baseRtt
    if (m_baseRtt != Time::Max() && rtt < m_baseRtt * 0.90)
    {
        NS_LOG_DEBUG("reset");
        m_baseRtt = rtt;
        m_minRtt = rtt;
        m_cntRtt = 0;
        return;
    }

    // Allow baseRtt to decrease when RTTs improve
    if (rtt < m_baseRtt)
        m_baseRtt = rtt;

  
     m_minRtt = std::min(m_minRtt, rtt);
     //NS_LOG_DEBUG("Updated m_minRtt = " << m_minRtt);
  
     m_baseRtt = std::min(m_baseRtt, rtt);
     //NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);
  
     
     m_cntRtt++;
     //NS_LOG_DEBUG("Updated m_cntRtt = " << m_cntRtt);
    
}
 
 void
 TcpQuickVegas::EnableVegas(Ptr<TcpSocketState> tcb)
 {
     NS_LOG_FUNCTION(this << tcb);
 
     m_doingVegasNow = true;
     m_begSndNxt = tcb->m_nextTxSequence;
     m_cntRtt = 0;
     m_minRtt = Time::Max();
 }
 
 void
 TcpQuickVegas::DisableVegas()
 {
     NS_LOG_FUNCTION(this);
 
     m_doingVegasNow = false;
 }
 
 void
 TcpQuickVegas::CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
 {
     NS_LOG_DEBUG("Disabled");

     NS_LOG_FUNCTION(this << tcb << newState);
     if (newState == TcpSocketState::CA_OPEN)
     {
         EnableVegas(tcb);
     }
     else
     {
         DisableVegas();
     }
 }


// showed it to sir, with 2 cbr traffic

 void
 TcpQuickVegas::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
 {
     //NS_LOG_FUNCTION(this << tcb << segmentsAcked);

     //NS_LOG_DEBUG("IncreaseWindow");
 
     if (!m_doingVegasNow)
     {
         
         NS_LOG_LOGIC("QuickVegas is not turned on, we follow Vegas algorithm.");

         //TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
         TcpVegas::IncreaseWindow(tcb, segmentsAcked);

         return;
     }

     
    
    if (tcb->m_lastAckedSeq >= m_begSndNxt)
    { 

        //NS_LOG_LOGIC("A Vegas cycle has finished, we adjust cwnd once per RTT.");

        m_begSndNxt = tcb->m_nextTxSequence;

        
        if (m_cntRtt <= 2)
        {   
            //NS_LOG_LOGIC("We do not have enough RTT samples to do Vegas, so we behave like NewReno.");
            TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        }else{

            //NS_LOG_LOGIC("We have enough RTT samples to perform Vegas calculations");
            

            uint32_t diff;
            uint32_t targetCwnd;
            uint32_t segCwnd = tcb->GetCwndInSegments();
            double tmp = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
            targetCwnd = static_cast<uint32_t>(segCwnd * tmp);
            diff = segCwnd - targetCwnd;

            //NS_LOG_DEBUG("diff = " << diff);


            if (diff > m_gamma && (tcb->m_cWnd < tcb->m_ssThresh))
            {

                tcb->m_ssThresh = tcb->m_cWnd - (tcb->m_cWnd / 8);
                tcb->m_cWnd = tcb->m_ssThresh;
                m_succ = 0;
                m_incr = 0;
                //NS_LOG_DEBUG("Exiting slow start, ssThresh = " << tcb->m_ssThresh);

            } else if (tcb->m_cWnd < tcb->m_ssThresh)
            {   
                // Slow start mode
                NS_LOG_LOGIC("We are in slow start and diff < m_gamma, so we "
                             "follow NewReno slow start");

                NS_LOG_DEBUG("still in Slow start");
                TcpVegas::SlowStart(tcb, segmentsAcked);

            }else{
                if(diff > static_cast<int32_t>(m_beta)){
                    uint32_t target = (m_alpha + m_beta) / 2;
                    uint32_t decrease = diff - target;
                    uint32_t newSegCwnd;
                    if(decrease >= segCwnd){  
                        newSegCwnd = 2;
                    }else{
                        newSegCwnd = segCwnd - decrease;  
                    }
                    tcb->m_cWnd = std::max(newSegCwnd * tcb->m_segmentSize, 2 * tcb->m_segmentSize);
                    m_incr = 0;
                    m_succ = 0;

                    //NS_LOG_DEBUG("GREATER than Beta");
                    //NS_LOG_DEBUG("Decreasing cwnd by " << decrease << " segments");
                    
                }else if(diff < static_cast<int32_t>(m_alpha)){
                    m_succ++;
                    uint32_t maxIncrease = segCwnd;
                    uint32_t actualIncrease = (m_beta - diff) * m_succ;
                    if(actualIncrease > maxIncrease){
                        m_incr = 1;
                    }else{
                        //m_incr = static_cast<double>(actualIncrease) / static_cast<double>(segCwnd);
                        m_incr = actualIncrease;
                    }

                    //NS_LOG_DEBUG(", increase =" << actualIncrease);
                    
                    tcb->m_cWnd += m_incr * tcb->m_segmentSize; //not mentioned
                    //NS_LOG_DEBUG("Less than ALPHA");
        
                    //NS_LOG_DEBUG("Increasing: succ = " << m_succ << ", incr = " << m_incr);
                    
                }else if(diff > static_cast<int32_t>((m_alpha + m_beta) / 2)){
                    uint32_t decrease = 1;
                    uint32_t newSegCwnd;
                    if(decrease >= segCwnd){  
                        newSegCwnd = 2;
                    }else{
                        newSegCwnd = segCwnd - decrease;  
                    }
                    tcb->m_cWnd = std::max(newSegCwnd * tcb->m_segmentSize, 2 * tcb->m_segmentSize);
            
                    m_incr = 0;
                    m_succ = 0;
                    
                }else if(diff < static_cast<int32_t>((m_alpha + m_beta) / 2)){
                    
                    m_incr = 1.0 / static_cast<double>(segCwnd); 
                    tcb->m_cWnd += m_incr * tcb->m_segmentSize; //not mentioned
                    m_succ = 0;
                    
                }else{
                    m_incr = 0;
                    m_succ = 0;
                }

            }

            tcb->m_ssThresh = std::max(tcb->m_ssThresh, 3 * tcb->m_cWnd / 4);
            //NS_LOG_DEBUG("Updated ssThresh = " << tcb->m_ssThresh);

        }

        
        m_cntRtt = 0;
        m_minRtt = Time::Max();

        

        
    }else if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        TcpVegas::SlowStart(tcb, segmentsAcked);
    }

        
}



// this for higher throuphput
// void
// TcpQuickVegas::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
// {
//     if (!m_doingVegasNow)
//     {
//         TcpVegas::IncreaseWindow(tcb, segmentsAcked);
//         return;
//     }

//     if (tcb->m_lastAckedSeq >= m_begSndNxt)
//     {
//         m_begSndNxt = tcb->m_nextTxSequence;

//         if (m_cntRtt <= 2)
//         {
//             TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
//         }
//         else
//         {
//             uint32_t segCwnd = tcb->GetCwndInSegments();
//             double   tmp     = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();

//             // BUG 1 FIX: use signed integer so underflow cannot wrap
//             int32_t targetCwnd = static_cast<int32_t>(segCwnd * tmp);
//             int32_t diff       = static_cast<int32_t>(segCwnd) - targetCwnd;
//             // clamp: diff cannot be negative (means no queuing at all)
//             if (diff < 0) diff = 0;

//             int32_t alpha = static_cast<int32_t>(m_alpha);
//             int32_t beta  = static_cast<int32_t>(m_beta);
//             int32_t mid   = (alpha + beta) / 2;  // = 3 with defaults

//             // --- Slow-start handling ---
//             if (tcb->m_cWnd < tcb->m_ssThresh)
//             {
//                 if (diff > static_cast<int32_t>(m_gamma))
//                 {
//                     // Exit slow start early (Vegas-style)
//                     tcb->m_ssThresh = tcb->m_cWnd - (tcb->m_cWnd / 8);
//                     tcb->m_cWnd     = tcb->m_ssThresh;
//                     m_succ = 0;
//                     m_incr = 0;
//                 }
//                 else
//                 {
//                     TcpNewReno::SlowStart(tcb, segmentsAcked);
//                 }
//             }
//             else
//             {
//                 // --- Congestion avoidance: Quick Vegas window update ---
//                 // BUG 3 FIX: always reset m_succ in the hold/decrease zones

//                 if (diff > beta)
//                 {
//                     // Eq. (4): decrement by (diff - mid)
//                     int32_t decrease = diff - mid;
//                     if (decrease < 1) decrease = 1;
//                     int32_t newSeg = static_cast<int32_t>(segCwnd) - decrease;
//                     if (newSeg < 2) newSeg = 2;
//                     tcb->m_cWnd = static_cast<uint32_t>(newSeg) * tcb->m_segmentSize;
//                     m_succ = 0;
//                     m_incr = 0;
//                 }
//                 else if (diff < alpha)
//                 {
//                     // Eq. (3): increment = (beta - diff) * succ, capped at doubling
//                     m_succ++;
//                     int32_t inc = (beta - diff) * static_cast<int32_t>(m_succ);
//                     if (inc > static_cast<int32_t>(segCwnd))
//                         inc = static_cast<int32_t>(segCwnd); // cap: at most double

//                     // BUG 2 FIX: spread increment over ACKs this RTT via m_incrAccum
//                     // Store total segments to add; apply 1 segment per
//                     // (segCwnd / inc) ACKs using the accumulator in PktsAcked
//                     m_incr = inc;  // total segments to add this RTT
//                     tcb->m_cWnd += static_cast<uint32_t>(m_incr) * tcb->m_segmentSize;
//                     m_incr = 0;   // already applied for this RTT
//                     // (for per-ACK spreading, move the line above to PktsAcked)
//                 }
//                 else if (diff > mid)
//                 {
//                     // Between mid and beta: gentle -1
//                     if (segCwnd > 2)
//                         tcb->m_cWnd -= tcb->m_segmentSize;
//                     m_succ = 0;  // BUG 3 FIX
//                     m_incr = 0;
//                 }
//                 else if (diff < mid)
//                 {
//                     // Between alpha and mid: gentle +1
//                     tcb->m_cWnd += tcb->m_segmentSize;
//                     m_succ = 0;  // BUG 3 FIX
//                     m_incr = 0;
//                 }
//                 else
//                 {
//                     // diff == mid: at equilibrium
//                     m_succ = 0;
//                     m_incr = 0;
//                 }
//             }
//             // BUG 4 FIX: removed the bad ssThresh line entirely
//         }

//         m_cntRtt = 0;
//         m_minRtt = Time::Max();
//     }
//     else if (tcb->m_cWnd < tcb->m_ssThresh)
//     {
//         TcpNewReno::SlowStart(tcb, segmentsAcked);
//     }
// }

 


// with gallop

// void
// TcpQuickVegas::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
// {
//     if (!m_doingVegasNow)
//     {
//         TcpVegas::IncreaseWindow(tcb, segmentsAcked);
//         return;
//     }

//     if (tcb->m_lastAckedSeq >= m_begSndNxt)
//     {
//         m_begSndNxt = tcb->m_nextTxSequence;

//         if (m_cntRtt <= 2)
//         {
//             TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
//         }
//         else
//         {
//             uint32_t segCwnd = tcb->GetCwndInSegments();

//             // i added to track last incr
//             uint32_t m_maxincr = segCwnd;

//             double   tmp     = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();

//             int32_t targetCwnd = static_cast<int32_t>(segCwnd * tmp);
//             int32_t diff       = static_cast<int32_t>(segCwnd) - targetCwnd;
            
//             if (diff < 0) diff = 0;

//             int32_t alpha = static_cast<int32_t>(m_alpha);
//             int32_t beta  = static_cast<int32_t>(m_beta);
//             int32_t mid   = (alpha + beta) / 2;  

//             // --- Slow-start handling ---
//             if (tcb->m_cWnd < tcb->m_ssThresh)
//             {
//                 if (diff > static_cast<int32_t>(m_gamma))
//                 {

//                     if(diff >= static_cast<int32_t>(m_beta)){
//                          // cwnd -= (lastIncr + delta - beta)
//                         uint32_t cut = m_lastIncr + static_cast<uint32_t>(diff - static_cast<int32_t>(m_beta));
//                         int32_t newCwndPkts = static_cast<int32_t>(segCwnd) - static_cast<int32_t>(cut);
//                         if (newCwndPkts < 2) newCwndPkts = 2;

//                         tcb->m_cWnd = static_cast<uint32_t>(newCwndPkts) * tcb->m_segmentSize;
//                         tcb->m_ssThresh = 2 * tcb->m_segmentSize;
//                         m_status = 2;
//                     }else{

//                         if (m_status == 0)
//                         {
//                             m_incr_s = m_incr_s / 2;
//                             if (m_incr_s <= 1)
//                             {
//                                 m_incr_s          = 1;
//                                 tcb->m_ssThresh = 2 * tcb->m_segmentSize;
//                             }
//                             m_status = 1;
//                         }
//                         else
//                         {
//                             m_status = 0;
//                         }
//                         tcb->m_cWnd += m_incr_s * tcb->m_segmentSize;

//                     }
                    
//                 }else 
//                 {
//                     tcb->m_cWnd += m_incr_s * tcb->m_segmentSize;
//                     if (m_incr_s < m_maxincr) m_incr_s++;
//                 }

//                  // at the end of the RTT window, record what was actually added
//                  m_lastIncr = tcb->GetCwndInSegments() - segCwnd;
//             }
//             else
//             {
//                 // --- Congestion avoidance: Quick Vegas window update ---

//                 if (diff > beta)
//                 {
                  
//                     int32_t decrease = diff - mid;
//                     if (decrease < 1) decrease = 1;
//                     int32_t newSeg = static_cast<int32_t>(segCwnd) - decrease;
//                     if (newSeg < 2) newSeg = 2;
//                     tcb->m_cWnd = static_cast<uint32_t>(newSeg) * tcb->m_segmentSize;
//                     m_succ = 0;
//                     m_incr = 0;
//                 }
//                 else if (diff < alpha)
//                 {
                   
//                     m_succ++;
//                     int32_t inc = (beta - diff) * static_cast<int32_t>(m_succ);
//                     if (inc > static_cast<int32_t>(segCwnd))
//                         inc = static_cast<int32_t>(segCwnd);

//                     m_incr = inc;  
//                     tcb->m_cWnd += static_cast<uint32_t>(m_incr) * tcb->m_segmentSize;
//                     m_incr = 0;  
             
//                 }
//                 else if (diff > mid)
//                 {
//                     if (segCwnd > 2)
//                         tcb->m_cWnd -= tcb->m_segmentSize;
//                     m_succ = 0;  
//                     m_incr = 0;
//                 }
//                 else if (diff < mid)
//                 {
//                     tcb->m_cWnd += tcb->m_segmentSize;
//                     m_succ = 0; 
//                     m_incr = 0;
//                 }
//                 else
//                 {
                   
//                     m_succ = 0;
//                     m_incr = 0;
//                 }
//             }

           
//         }

        

//         m_cntRtt = 0;
//         m_minRtt = Time::Max();
//     }
//     else if (tcb->m_cWnd < tcb->m_ssThresh)
//     {
//         TcpNewReno::SlowStart(tcb, segmentsAcked);
//     }
// }
 
 std::string
 TcpQuickVegas::GetName() const
 {
     return "TcpQuickVegas";
 }
 
 uint32_t
 TcpQuickVegas::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
 {
     NS_LOG_FUNCTION(this << tcb << bytesInFlight);
     //return std::max(std::min(tcb->m_ssThresh.Get(), tcb->m_cWnd.Get() - tcb->m_segmentSize),
                     //2 * tcb->m_segmentSize);

     // here i added
     // Return 3/4 or 1/2 of cwnd depending on retransmission count
     // (Similar to Vegas behavior described in paper)
     return std::max<uint32_t>(2 * tcb->m_segmentSize, tcb->m_cWnd / 2);
 }
 
 } // namespace ns3
 