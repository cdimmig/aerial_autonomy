#include "aerial_autonomy/controller_connectors/qrotor_backstepping_controller_connector.h"
#include <glog/logging.h>
#include <tf_conversions/tf_eigen.h>

bool QrotorBacksteppingControllerConnector::extractSensorData(
    std::pair<double, QrotorBacksteppingState> &sensor_data) {

  drone_hardware_.getquaddata(data_);

  std::chrono::duration<double> time_duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(
          std::chrono::high_resolution_clock::now() - t_0_);
  double current_time = time_duration.count();

  tf::Quaternion q = tf::createQuaternionFromRPY(
      data_.rpydata.x, data_.rpydata.y, data_.rpydata.z);

  current_state_.pose = tf::Transform(
      q, tf::Vector3(data_.localpos.x, data_.localpos.y, data_.localpos.z));

  current_state_.v =
      tf::Vector3(data_.linvel.x, data_.linvel.y, data_.linvel.z);

  current_state_.w = tf::Vector3(data_.omega.x, data_.omega.y, data_.omega.z);

  current_state_.thrust = thrust_;

  current_state_.thrust_dot = thrust_dot_;

  sensor_data = std::make_pair(current_time, current_state_);

  thrust_gain_estimator_.addSensorData(data_.rpydata.x, data_.rpydata.y,
                                       data_.linacc.z);

  return true;
}

void QrotorBacksteppingControllerConnector::sendControllerCommands(
    QrotorBacksteppingControl control) {
  double Thrust_ddot = control.thrust_ddot;
  tf::Vector3 Torque = control.torque;
  std::chrono::time_point<std::chrono::high_resolution_clock> current_time =
      std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> dt_duration =
      std::chrono::duration_cast<std::chrono::duration<double>>(current_time -
                                                                previous_time_);
  double dt = dt_duration.count();
  previous_time_ = current_time;
  thrust_dot_ += Thrust_ddot * dt;
  thrust_ += thrust_dot_ * dt;

  // conversions
  Eigen::Vector3d torque;
  tf::vectorTFToEigen(Torque, torque);
  Eigen::Vector3d current_omega;
  current_omega << data_.omega.x, data_.omega.y, data_.omega.z;
  Eigen::Vector3d current_rpy;
  current_rpy << data_.rpydata.x, data_.rpydata.y, data_.rpydata.z;

  Eigen::Vector3d omega_dot_cmd =
      J_.llt().solve((J_ * current_omega).cross(current_omega) + torque);
  omega_cmd_ += omega_dot_cmd * dt;
  Eigen::Vector3d rpydot_cmd = omegaToRpyDot(omega_cmd_, current_rpy);
  roll_cmd_ += rpydot_cmd[0] * dt;  // Roll command
  pitch_cmd_ += rpydot_cmd[1] * dt; // Pitch command
  yaw_rate_cmd_ = rpydot_cmd[2];    // Yaw rate command

  // Bounds
  if (roll_cmd_ < lb_(0)) {
    roll_cmd_ = lb_(0);
  } else if (roll_cmd_ > ub_(0)) {
    roll_cmd_ = ub_(0);
  }
  if (pitch_cmd_ < lb_(1)) {
    pitch_cmd_ = lb_(1);
  } else if (pitch_cmd_ > ub_(1)) {
    pitch_cmd_ = ub_(1);
  }
  if (yaw_rate_cmd_ < lb_(2)) {
    yaw_rate_cmd_ = lb_(2);
  } else if (yaw_rate_cmd_ > ub_(2)) {
    yaw_rate_cmd_ = ub_(2);
  }
  if (thrust_ / m_ < lb_(3) * g_) {
    thrust_ = m_ * lb_(3) * g_;
  } else if (thrust_ / m_ > ub_(3) * g_) {
    thrust_ = m_ * ub_(3) * g_;
  }

  thrust_cmd_ =
      thrust_ / (m_ * thrust_gain_estimator_.getThrustGain()); // thrust_command

  geometry_msgs::Quaternion rpyt_msg;
  rpyt_msg.x = roll_cmd_;
  rpyt_msg.y = pitch_cmd_;
  rpyt_msg.z = yaw_rate_cmd_;
  rpyt_msg.w = thrust_cmd_;
  thrust_gain_estimator_.addThrustCommand(rpyt_msg.w);
  drone_hardware_.cmdrpyawratethrust(rpyt_msg);

  DATA_LOG("qrotor_backstepping_controller_connector")
      << roll_cmd_ << pitch_cmd_ << yaw_rate_cmd_ << thrust_cmd_
      << DataStream::endl;
}

void QrotorBacksteppingControllerConnector::setGoal(
    std::shared_ptr<ReferenceTrajectory<ParticleState, Snap>> goal) {
  BaseClass::setGoal(goal);
  t_0_ = std::chrono::high_resolution_clock::now();
  previous_time_ = std::chrono::high_resolution_clock::now();
  thrust_ = m_ * g_;
  thrust_dot_ = 0.0;
  roll_cmd_ = 0;
  pitch_cmd_ = 0;
  omega_cmd_ << 0, 0, 0;
  lb_ << -0.785, -0.785, -1.5708, 0.8;
  ub_ << 0.785, 0.785, 1.5708, 1.2;

  VLOG(1) << "Clearing thrust estimator buffer";
  thrust_gain_estimator_.clearBuffer();
}

Eigen::Vector3d QrotorBacksteppingControllerConnector::omegaToRpyDot(
    const Eigen::Vector3d &omega, const Eigen::Vector3d &rpy) {
  Eigen::Matrix3d Mrpy;
  double s_roll = sin(rpy[0]), c_roll = cos(rpy[0]), t_pitch = tan(rpy[1]);
  double sec_pitch = sqrt(1 + t_pitch * t_pitch);
  Mrpy << 1, s_roll * t_pitch, c_roll * t_pitch, 0, c_roll, -s_roll, 0,
      s_roll * sec_pitch, c_roll * sec_pitch;
  return Mrpy * omega;
}