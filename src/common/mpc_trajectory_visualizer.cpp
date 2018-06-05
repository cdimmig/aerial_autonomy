#include "aerial_autonomy/common/mpc_trajectory_visualizer.h"

MPCTrajectoryVisualizer::MPCTrajectoryVisualizer(ControllerConnector &connector,
                                                 MPCVisualizerConfig config)
    : connector_(connector), nh_("mpc_visualizer"),
      visualizer_(nh_, config.parent_frame(), config.visualize_velocities()),
      config_(config) {}

void MPCTrajectoryVisualizer::publishTrajectory() {
  ControllerStatus status = connector_.getStatus();
  if (status == ControllerStatus::Active ||
      status == ControllerStatus::Completed) {
    connector_.getTrajectory(xs_, us_);
    gcop_comm::CtrlTraj trajectory =
        getTrajectory(xs_, us_, config_.skip_segments());
    connector_.getDesiredTrajectory(xds_, uds_);
    gcop_comm::CtrlTraj desired_trajectory =
        getTrajectory(xds_, uds_, config_.skip_segments());
    const auto &trajectory_color = config_.trajectory_color();
    visualizer_.setID(config_.trajectory_id());
    visualizer_.setColorLineStrip(trajectory_color.r(), trajectory_color.b(),
                                  trajectory_color.a());
    visualizer_.publishTrajectory(trajectory);
    const auto &desired_trajectory_color = config_.desired_trajectory_color();
    visualizer_.setID(config_.desired_trajectory_id());
    visualizer_.setColorLineStrip(desired_trajectory_color.r(),
                                  desired_trajectory_color.b(),
                                  desired_trajectory_color.a());
    visualizer_.publishLineStrip(desired_trajectory);
  }
}

gcop_comm::CtrlTraj
MPCTrajectoryVisualizer::getTrajectory(std::vector<Eigen::VectorXd> &xs,
                                       std::vector<Eigen::VectorXd> &us,
                                       int skip_segments) {
  gcop_comm::CtrlTraj control_trajectory;
  control_trajectory.N = us.size();
  for (unsigned int i = 0; i < (control_trajectory.N + 1); i += skip_segments) {
    const auto &x = xs[i];
    gcop_comm::State state;
    state.basepose.translation.x = x[0];
    state.basepose.translation.y = x[1];
    state.basepose.translation.z = x[2];
    tf::Quaternion quat = tf::createQuaternionFromRPY(x[3], x[4], x[5]);
    tf::quaternionTFToMsg(quat, state.basepose.rotation);
    state.basetwist.linear.x = x[6];
    state.basetwist.linear.y = x[7];
    state.basetwist.linear.z = x[8];
    for (int j = 0; j < 4; ++j) {
      state.statevector.push_back(x[15 + j]);
    }
    control_trajectory.statemsg.push_back(state);
    if (i < control_trajectory.N) {
      gcop_comm::Ctrl control;
      control.ctrlvec[0] = us[i][0]; // thrust
      for (int j = 0; j < 3; ++j) {
        control.ctrlvec[j + 1] = x[12 + j];
      }
      // Desired joint angles
      control.ctrlvec[4] = x[19];
      control.ctrlvec[4] = x[20];
      control_trajectory.ctrl.push_back(control);
    }
  }
  return control_trajectory;
}