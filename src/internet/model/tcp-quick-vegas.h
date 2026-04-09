
//  #ifndef TcpQuickVegas_H
//  #define TcpQuickVegas_H
 
//  #include "tcp-vegas.h"
 
//  namespace ns3
//  {
 
//  class TcpSocketState;
 
//  /**
//   * @ingroup congestionOps
//   *
//   * @brief An implementation of TCP Vegas
//   *
//   * TCP Vegas is a pure delay-based congestion control algorithm implementing a proactive
//   * scheme that tries to prevent packet drops by maintaining a small backlog at the
//   * bottleneck queue.
//   *
//   * Vegas continuously measures the actual throughput a connection achieves as shown in
//   * Equation (1) and compares it with the expected throughput calculated in Equation (2).
//   * The difference between these 2 sending rates in Equation (3) reflects the amount of
//   * extra packets being queued at the bottleneck.
//   *
//   *              actual = cwnd / RTT        (1)
//   *              expected = cwnd / BaseRTT  (2)
//   *              diff = expected - actual   (3)
//   *
//   * To avoid congestion, Vegas linearly increases/decreases its congestion window to ensure
//   * the diff value fall between the 2 predefined thresholds, alpha and beta.
//   * diff and another threshold, gamma, are used to determine when Vegas should change from
//   * its slow-start mode to linear increase/decrease mode.
//   *
//   * Following the implementation of Vegas in Linux, we use 2, 4, and 1 as the default values
//   * of alpha, beta, and gamma, respectively.
//   *
//   * More information: http://dx.doi.org/10.1109/49.464716
//   */
 
//  class TcpQuickVegas : public TcpVegas
//  {
//    public:
//      /**
//       * @brief Get the type ID.
//       * @return the object TypeId
//       */
//      static TypeId GetTypeId();
 
//      /**
//       * Create an unbound tcp socket.
//       */
//      TcpQuickVegas();
 
//      /**
//       * @brief Copy constructor
//       * @param sock the object to copy
//       */
//      TcpQuickVegas(const TcpQuickVegas& sock);
//      ~TcpQuickVegas() override;
 
//      std::string GetName() const override;
 
//      /**
//       * @brief Compute RTTs needed to execute Vegas algorithm
//       *
//       * The function filters RTT samples from the last RTT to find
//       * the current smallest propagation delay + queueing delay (minRtt).
//       * We take the minimum to avoid the effects of delayed ACKs.
//       *
//       * The function also min-filters all RTT measurements seen to find the
//       * propagation delay (baseRtt).
//       *
//       * @param tcb internal congestion state
//       * @param segmentsAcked count of segments ACKed
//       * @param rtt last RTT
//       *
//       */
//      void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
 
//      /**
//       * @brief Enable/disable Vegas algorithm depending on the congestion state
//       *
//       * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
//       *
//       * @param tcb internal congestion state
//       * @param newState new congestion state to which the TCP is going to switch
//       */
//      void CongestionStateSet(Ptr<TcpSocketState> tcb,
//                              const TcpSocketState::TcpCongState_t newState) override;
 
//      /**
//       * @brief Adjust cwnd following Vegas linear increase/decrease algorithm
//       *
//       * @param tcb internal congestion state
//       * @param segmentsAcked count of segments ACKed
//       */
//      void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
 
//      /**
//       * @brief Get slow start threshold following Vegas principle
//       *
//       * @param tcb internal congestion state
//       * @param bytesInFlight bytes in flight
//       *
//       * @return the slow start threshold value
//       */
//      uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
 
//      Ptr<TcpCongestionOps> Fork() override;
 
//    protected:
//    private:
//      /**
//       * @brief Enable Vegas algorithm to start taking Vegas samples
//       *
//       * Vegas algorithm is enabled in the following situations:
//       * 1. at the establishment of a connection
//       * 2. after an RTO
//       * 3. after fast recovery
//       * 4. when an idle connection is restarted
//       *
//       * @param tcb internal congestion state
//       */
//      void EnableVegas(Ptr<TcpSocketState> tcb);
 
//      /**
//       * @brief Stop taking Vegas samples
//       */
//      void DisableVegas();



//      // improvement
//     uint32_t ComputeAdaptiveGamma() const;
 
//    private:
//      uint32_t m_alpha;             //!< Alpha threshold, lower bound of packets in network
//      uint32_t m_beta;              //!< Beta threshold, upper bound of packets in network
//      uint32_t m_gamma;             //!< Gamma threshold, limit on increase
//      Time m_baseRtt;               //!< Minimum of all Vegas RTT measurements seen during connection
//      Time m_minRtt;                //!< Minimum of all RTT measurements within last RTT
//      uint32_t m_cntRtt;            //!< Number of RTT measurements during last RTT
//      bool m_doingVegasNow;         //!< If true, do Vegas for this RTT
//      SequenceNumber32 m_begSndNxt; //!< Right edge during last RTT

//     // Quick Vegas
//     uint32_t m_succ;            // Consecutive successful increments
//     uint32_t m_incr;            // Increment amount per ACK
//     uint32_t m_rttRounds;
//     uint32_t m_status;
//     uint32_t m_lastIncr;
//     uint32_t m_incr_s;

//      // improvement
//      double   m_rttSum;     //!< Sum of RTT samples in current RTT round (seconds)
//      double   m_rttSumSq;   //!< Sum of squared RTT samples (for variance)
//      uint32_t m_rttCount;   //!< Number of RTT samples accumulated this round
//      uint32_t m_rttRoundCount; //!< RTT rounds since last stats flush
//  };
 
//  } // namespace ns3
 
//  #endif // TcpQuickVegas_H
 






#ifndef TcpQuickVegas_H
#define TcpQuickVegas_H

#include "tcp-vegas.h"

namespace ns3
{

class TcpSocketState;

/**
 * @ingroup congestionOps
 *
 * @brief An implementation of TCP Vegas
 *
 * TCP Vegas is a pure delay-based congestion control algorithm implementing a proactive
 * scheme that tries to prevent packet drops by maintaining a small backlog at the
 * bottleneck queue.
 *
 * Vegas continuously measures the actual throughput a connection achieves as shown in
 * Equation (1) and compares it with the expected throughput calculated in Equation (2).
 * The difference between these 2 sending rates in Equation (3) reflects the amount of
 * extra packets being queued at the bottleneck.
 *
 *              actual = cwnd / RTT        (1)
 *              expected = cwnd / BaseRTT  (2)
 *              diff = expected - actual   (3)
 *
 * To avoid congestion, Vegas linearly increases/decreases its congestion window to ensure
 * the diff value fall between the 2 predefined thresholds, alpha and beta.
 * diff and another threshold, gamma, are used to determine when Vegas should change from
 * its slow-start mode to linear increase/decrease mode.
 *
 * Following the implementation of Vegas in Linux, we use 2, 4, and 1 as the default values
 * of alpha, beta, and gamma, respectively.
 *
 * More information: http://dx.doi.org/10.1109/49.464716
 */

class TcpQuickVegas : public TcpVegas
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create an unbound tcp socket.
     */
    TcpQuickVegas();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpQuickVegas(const TcpQuickVegas& sock);
    ~TcpQuickVegas() override;

    std::string GetName() const override;

    /**
     * @brief Compute RTTs needed to execute Vegas algorithm
     *
     * The function filters RTT samples from the last RTT to find
     * the current smallest propagation delay + queueing delay (minRtt).
     * We take the minimum to avoid the effects of delayed ACKs.
     *
     * The function also min-filters all RTT measurements seen to find the
     * propagation delay (baseRtt).
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     * @param rtt last RTT
     *
     */
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;

    /**
     * @brief Enable/disable Vegas algorithm depending on the congestion state
     *
     * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
     *
     * @param tcb internal congestion state
     * @param newState new congestion state to which the TCP is going to switch
     */
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;

    /**
     * @brief Adjust cwnd following Vegas linear increase/decrease algorithm
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     */
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

    /**
     * @brief Get slow start threshold following Vegas principle
     *
     * @param tcb internal congestion state
     * @param bytesInFlight bytes in flight
     *
     * @return the slow start threshold value
     */
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    Ptr<TcpCongestionOps> Fork() override;

  protected:
  private:
    /**
     * @brief Enable Vegas algorithm to start taking Vegas samples
     *
     * Vegas algorithm is enabled in the following situations:
     * 1. at the establishment of a connection
     * 2. after an RTO
     * 3. after fast recovery
     * 4. when an idle connection is restarted
     *
     * @param tcb internal congestion state
     */
    void EnableVegas(Ptr<TcpSocketState> tcb);

    /**
     * @brief Stop taking Vegas samples
     */
    void DisableVegas();



    // improvement
   uint32_t ComputeAdaptiveGamma() const;

  private:
    uint32_t m_alpha;             //!< Alpha threshold, lower bound of packets in network
    uint32_t m_beta;              //!< Beta threshold, upper bound of packets in network
    uint32_t m_gamma;             //!< Gamma threshold, limit on increase
   uint32_t m_slowStartMode;     //!< 1: legacy (diff > gamma), 2: trend-gated exit
    Time m_baseRtt;               //!< Minimum of all Vegas RTT measurements seen during connection
    Time m_minRtt;                //!< Minimum of all RTT measurements within last RTT
    uint32_t m_cntRtt;            //!< Number of RTT measurements during last RTT
    bool m_doingVegasNow;         //!< If true, do Vegas for this RTT
    SequenceNumber32 m_begSndNxt; //!< Right edge during last RTT

   // Quick Vegas
   uint32_t m_succ;            // Consecutive successful increments
   uint32_t m_incr;            // Increment amount per ACK
   uint32_t m_rttRounds;
   uint32_t m_status;
   uint32_t m_lastIncr;
   uint32_t m_incr_s;

    // improvement
    double   m_rttSum;     
    double   m_rttSumSq;   
    uint32_t m_rttCount;   
    uint32_t m_rttRoundCount; 

   Time m_prevRoundMinRtt; 
   uint32_t m_risingTrendRounds; 
};

} // namespace ns3

#endif // TcpQuickVegas_H


