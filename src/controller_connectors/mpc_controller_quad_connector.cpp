#include <aerial_autonomy/common/conversions.h>
#include <aerial_autonomy/common/math.h>
#include <aerial_autonomy/controller_connectors/mpc_controller_quad_connector.h>
#include <aerial_autonomy/log/log.h>

constexpr int MPCControllerQuadConnector::state_size_;

MPCControllerQuadConnector::MPCControllerQuadConnector(
    parsernode::Parser &drone_hardware,
    AbstractMPCController<StateType, ControlType> &controller,
    ThrustGainEstimator &thrust_gain_estimator, int delay_buffer_size,
    MPCConnectorConfig config, SensorPtr<tf::StampedTransform> pose_sensor,
    AbstractConstraintGeneratorPtr constraint_generator)
    : BaseMPCControllerQuadConnector(
          drone_hardware, controller, thrust_gain_estimator, delay_buffer_size,
          config, pose_sensor, constraint_generator) {
  // clang-format off
  DATA_HEADER("quad_mpc_state_estimator") << "x" << "y" << "z"
                                          << "vx" << "vy" << "vz"
                                          << "r" << "p" << "y"
                                          << "rdot" << "pdot" << "ydot"
                                          << "rd" << "pd" << "yd"
                                          << "kt" << DataStream::endl;
  // clang-format on
}

void MPCControllerQuadConnector::initialize() {
  initializePrivateController(private_controller_);
}

bool MPCControllerQuadConnector::estimateStateAndParameters(
    Eigen::VectorXd &current_state, Eigen::VectorXd &params) {
  double dt = getTimeDiff();
  current_state.resize(state_size_);
  bool result = fillQuadStateAndParameters(current_state, params, dt);
  if (result) {
    DATA_LOG("quad_mpc_state_estimator") << current_state << params[0]
                                         << DataStream::endl;
  }
  return result;
}