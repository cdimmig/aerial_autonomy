#pragma once
#include <memory>
#include <utility>
/**
* @brief An interface for retrieving states and controls from a trajectory
*
* @tparam StateT The type of state stored along the trajectory
* @tparam ControlT The type of control along the trajectory
*/
template <class StateT, class ControlT> class ReferenceTrajectory {
public:
  /**
  * @brief Gets the trajectory information at the specified time
  * @param t Time
  * @return Trajectory state and control
  */
  virtual std::pair<StateT, ControlT> atTime(double t) const = 0;

  /**
   * @brief goal for reference trajectory
   *
   * @param t Time when goal is asked for
   *
   * @return state at time t
   */
  virtual StateT goal(double t) { return atTime(t).first; }
};

/**
* @brief shared reference trajectory pointer
*
* @tparam StateT  state type
* @tparam ControlT control type
*/
template <class StateT, class ControlT>
using ReferenceTrajectoryPtr =
    std::shared_ptr<ReferenceTrajectory<StateT, ControlT>>;
