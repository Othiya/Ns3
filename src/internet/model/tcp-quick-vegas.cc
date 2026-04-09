 #include "tcp-quick-vegas.h"

 #include "tcp-socket-state.h"
 
 #include "ns3/log.h"
 //#include "ns3/simulator.h" // i added
 
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
       m_alpha(3),
       m_beta(5),
       m_gamma(2),
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
       m_incr_s(0),

       // improvement

       m_rttSum(0.0),
       m_rttSumSq(0.0),
       m_rttCount(0),
       m_rttRoundCount(0)
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
       m_incr_s(sock.m_incr_s),

        // improvement

        m_rttSum(sock.m_rttSum),
        m_rttSumSq(sock.m_rttSumSq),
        m_rttCount(sock.m_rttCount),
        m_rttRoundCount(sock.m_rttRoundCount)
       
       
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

     //improvement
     // only used in adaptive vegas

     double rttSec = rtt.GetSeconds();
     m_rttSum   += rttSec;
     m_rttSumSq += rttSec * rttSec;
     m_rttCount++;


    
    if (m_baseRtt != Time::Max() && rtt < m_baseRtt * 0.90)
    {
        NS_LOG_DEBUG("reset");
        m_baseRtt = rtt;
        m_minRtt = rtt;
        m_cntRtt = 0;
        return;
    }

    
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





// // this is base paper 

void
TcpQuickVegas::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{

    if (!m_doingVegasNow)
    {
        TcpVegas::IncreaseWindow(tcb, segmentsAcked);
        return;
    }

    if (tcb->m_lastAckedSeq >= m_begSndNxt)
    {
        m_begSndNxt = tcb->m_nextTxSequence;

        if (m_cntRtt <= 2)
        {
            TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        }
        else
        {
            uint32_t segCwnd = tcb->GetCwndInSegments();
            double   tmp     = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
            int32_t targetCwnd = static_cast<int32_t>(segCwnd * tmp);


            int32_t diff       = static_cast<int32_t>(segCwnd) - targetCwnd;
            if (diff < 0) diff = 0;

            int32_t alpha = static_cast<int32_t>(m_alpha);
            int32_t beta  = static_cast<int32_t>(m_beta);
            int32_t mid   = (alpha + beta) / 2;  

            m_succ = 0;
            m_incr = 0;

            // slow-start handling 
            if (diff > m_gamma && (tcb->m_cWnd < tcb->m_ssThresh))
            {
                
                    // Exit slow start early (Vegas-style)
                    tcb->m_ssThresh = tcb->m_cWnd - (tcb->m_cWnd / 8);
                    tcb->m_cWnd     = tcb->m_ssThresh;
                    
                
            }else if(tcb->m_cWnd < tcb->m_ssThresh){

                TcpNewReno::SlowStart(tcb, segmentsAcked);
            }
            else
            {
                // congestion avoidance
                
                if (diff > beta)
                {
                     
                     int32_t decrease = diff - mid;
                     if (decrease < 1) decrease = 1;
                     int32_t newSeg = static_cast<int32_t>(segCwnd) - decrease;
                     if (newSeg < 2) newSeg = 2;
                     tcb->m_cWnd = static_cast<uint32_t>(newSeg) * tcb->m_segmentSize;

                    // NS_LOG_DEBUG("ekhane komseeeeee" << newSeg);
                     m_succ = 0;
                     m_incr = 0;                     

                }
                else if (diff < alpha)
                {

                    m_succ++;
                    int32_t inc = (beta - diff) * static_cast<int32_t>(m_succ);
                    if (inc > static_cast<int32_t>(segCwnd))
                        inc = static_cast<int32_t>(segCwnd);

                    m_incr = inc;  
                    tcb->m_cWnd += static_cast<uint32_t>(m_incr) * tcb->m_segmentSize;
                    m_incr = 0;   
                   

                }
                else if (diff > mid)
                {
                    if (segCwnd > 2)
                        tcb->m_cWnd -= tcb->m_segmentSize;
                    m_succ = 0;  
                    m_incr = 0;
                }
                else if (diff < mid)
                {
                    tcb->m_cWnd += tcb->m_segmentSize;
                    m_succ = 0;  
                    m_incr = 0;
                }
                else
                {
                    
                    m_succ = 0;
                    m_incr = 0;
                }
            }
            
        }

        m_cntRtt = 0;
        m_minRtt = Time::Max();
    }
    else if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        TcpNewReno::SlowStart(tcb, segmentsAcked);
    }
}











// this for improvement

void
TcpQuickVegas::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
//     
// NS_LOG_DEBUG("Current values: alpha = " << m_alpha << ", beta = " << m_beta << ", gamma = " << m_gamma);
   if (!m_doingVegasNow)
   {
       TcpVegas::IncreaseWindow(tcb, segmentsAcked);
       return;
   }

   if (tcb->m_lastAckedSeq >= m_begSndNxt)
   {
       m_begSndNxt = tcb->m_nextTxSequence;

       if (m_cntRtt <= 2)
       {
           TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
       }
       else
       {
           uint32_t segCwnd = tcb->GetCwndInSegments();
           double   tmp     = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();

           int32_t targetCwnd = static_cast<int32_t>(segCwnd * tmp);
           int32_t diff       = static_cast<int32_t>(segCwnd) - targetCwnd;

           if (diff < 0) diff = 0;

           int32_t alpha = static_cast<int32_t>(m_alpha);
           int32_t beta  = static_cast<int32_t>(m_beta);
           int32_t mid   = (alpha + beta) / 2;

           m_succ = 0;
           m_incr = 0;

            //improvement
           bool exitSlowStart = (diff > static_cast<int32_t>(m_gamma));
           if (tcb->m_cWnd < tcb->m_ssThresh)
           {

                   const double segCwndDouble = std::max(2.0, static_cast<double>(segCwnd));
                   const double baseRttSec = std::max(m_baseRtt.GetSeconds(), 1e-9);
                   const double queueDelayRatio = std::max(0.0, (m_minRtt - m_baseRtt).GetSeconds() / baseRttSec);

                   const double rttGradientThreshold = std::clamp(static_cast<double>(m_alpha) / segCwndDouble, 0.01, 0.08);
                   const double queueDelayThreshold = std::clamp(static_cast<double>(m_gamma) / segCwndDouble, 0.02, 0.12);

                   double rttGradient = 0.0;
                   bool risingRttTrend = false;
                   if (m_prevRoundMinRtt != Time::Max())
                   {
                       rttGradient = (m_minRtt - m_prevRoundMinRtt).GetSeconds() / baseRttSec;
                       risingRttTrend = (rttGradient > rttGradientThreshold);
                   }

                   const bool persistentQueue = (queueDelayRatio > queueDelayThreshold);
                   const bool strongRttSpike = (rttGradient > 2.0 * rttGradientThreshold);

                   m_risingTrendRounds = risingRttTrend ? (m_risingTrendRounds + 1) : 0;
                   m_prevRoundMinRtt = m_minRtt;

                   exitSlowStart = exitSlowStart && persistentQueue && ((m_risingTrendRounds >= 2) || strongRttSpike);

           }

           // slow-start handling
           if ((tcb->m_cWnd < tcb->m_ssThresh) && exitSlowStart)
           {
               tcb->m_ssThresh = tcb->m_cWnd - (tcb->m_cWnd / 8);
               tcb->m_cWnd     = tcb->m_ssThresh;
           }
           else if (tcb->m_cWnd < tcb->m_ssThresh)
           {

               TcpNewReno::SlowStart(tcb, segmentsAcked);
           }
           else
           {
               // congestion avoidance

               if (diff > beta)
               {
            
                    int32_t decrease = diff - mid;
                    if (decrease < 1) decrease = 1;
                    int32_t newSeg = static_cast<int32_t>(segCwnd) - decrease;
                    if (newSeg < 2) newSeg = 2;
                    tcb->m_cWnd = static_cast<uint32_t>(newSeg) * tcb->m_segmentSize;

                    m_succ = 0;
                    m_incr = 0;                     

               }
               else if (diff < alpha)
               {
                   m_succ++;
                   int32_t inc = (beta - diff) * static_cast<int32_t>(m_succ);
                   if (inc > static_cast<int32_t>(segCwnd))
                       inc = static_cast<int32_t>(segCwnd);

                   m_incr = inc;  
                   tcb->m_cWnd += static_cast<uint32_t>(m_incr) * tcb->m_segmentSize;
                   m_incr = 0;  

               }
               else if (diff > mid)
               {
                   
                   if (segCwnd > 2)
                       tcb->m_cWnd -= tcb->m_segmentSize;
                   m_succ = 0;
                   m_incr = 0;
               }
               else if (diff < mid)
               {
                   tcb->m_cWnd += tcb->m_segmentSize;
                   m_succ = 0;  
                   m_incr = 0;
               }
               else
               {
                   m_succ = 0;
                   m_incr = 0;
               }
           }

       }

       m_cntRtt = 0;
       m_minRtt = Time::Max();
   }
   else if (tcb->m_cWnd < tcb->m_ssThresh)
   {
       TcpNewReno::SlowStart(tcb, segmentsAcked);
   }
}






 
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



















 









