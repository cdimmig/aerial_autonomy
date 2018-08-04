#include <aerial_autonomy/state_machines/visual_servoing_state_machine.h>
#include <aerial_autonomy/system_handlers/uav_vision_system_handler.h>
#include <aerial_autonomy/tests/test_utils.h>
#include <aerial_autonomy/visual_servoing_events.h>
#include <gtest/gtest.h>
#include <ros/ros.h>

#include <geometry_msgs/PoseStamped.h>
#include <std_msgs/String.h>

using namespace uav_basic_events;

class UAVVisionSystemHandlerTests : public ::testing::Test,
                                    public test_utils::BaseTestPubSubs {
public:
  UAVVisionSystemHandlerTests() : BaseTestPubSubs() {
    // Configure system
    UAVSystemHandlerConfig uav_system_handler_config;
    uav_system_handler_config.mutable_uav_system_config()
        ->set_minimum_takeoff_height(0.4);

    auto uav_config = uav_system_handler_config.mutable_uav_system_config();
    uav_config->set_uav_parser_type("quad_simulator_parser/QuadSimParser");
    // Position controller params
    auto pos_controller_config =
        uav_config->mutable_rpyt_based_position_controller_config()
            ->mutable_velocity_based_position_controller_config();
    pos_controller_config->set_position_gain(1.);
    pos_controller_config->set_z_gain(1.);
    pos_controller_config->set_yaw_gain(1.);
    pos_controller_config->set_max_velocity(2.);
    pos_controller_config->set_max_yaw_rate(5.);
    auto goal_position_tolerance =
        pos_controller_config->mutable_position_controller_config()
            ->mutable_goal_position_tolerance();
    pos_controller_config->mutable_position_controller_config()
        ->set_goal_yaw_tolerance(0.1);
    goal_position_tolerance->set_x(0.1);
    goal_position_tolerance->set_y(0.1);
    goal_position_tolerance->set_z(0.1);
    auto rpyt_vel_controller_tol =
        uav_config->mutable_rpyt_based_position_controller_config()
            ->mutable_rpyt_based_velocity_controller_config()
            ->mutable_velocity_controller_config()
            ->mutable_goal_velocity_tolerance();
    rpyt_vel_controller_tol->set_vx(0.1);
    rpyt_vel_controller_tol->set_vy(0.1);
    rpyt_vel_controller_tol->set_vz(0.1);
    // Fill MPC Config
    test_utils::fillMPCConfig(*uav_config);

    uav_system_handler_.reset(
        new UAVVisionSystemHandler<
            VisualServoingStateMachine,
            visual_servoing_events::VisualServoingEventManager<
                VisualServoingStateMachine>>(uav_system_handler_config,
                                             state_machine_config));
    ros::spinOnce();
  }

  const unsigned long timeout_wait =
      20; ///< Timeout for ros topic wait in seconds
  std::unique_ptr<
      UAVVisionSystemHandler<VisualServoingStateMachine,
                             visual_servoing_events::VisualServoingEventManager<
                                 VisualServoingStateMachine>>>
      uav_system_handler_; ///< system contains robot system, state machine
  BaseStateMachineConfig state_machine_config; ///< Passing by reference
                                               ///< cannot be temp variable
};

TEST_F(UAVVisionSystemHandlerTests, Constructor) {}

TEST_F(UAVVisionSystemHandlerTests, TestConnections) {
  while (!uav_system_handler_->isConnected()) {
  }
  SUCCEED();
}

TEST_F(UAVVisionSystemHandlerTests, ProcessEvents) {
  while (!uav_system_handler_->isConnected()) {
  }
  auto armed_true_fun = [=]() {
    ros::spinOnce();
    return uav_system_handler_->getUAVData().armed;
  };
  // Check takeoff works
  publishEvent("Takeoff");
  ASSERT_TRUE(test_utils::waitUntilTrue()(armed_true_fun,
                                          std::chrono::seconds(timeout_wait)));
  ASSERT_EQ(uav_system_handler_->getUAVData().localpos.z, 0.5);
  // Check subsequent event works
  publishEvent("Land");
  ASSERT_FALSE(test_utils::waitUntilFalse()(
      armed_true_fun, std::chrono::seconds(timeout_wait)));
  ASSERT_EQ(uav_system_handler_->getUAVData().localpos.z, 0.0);
}

TEST_F(UAVVisionSystemHandlerTests, ProcessPoseCommand) {
  while (!uav_system_handler_->isConnected()) {
  }
  auto armed_true_fun = [=]() {
    ros::spinOnce();
    return uav_system_handler_->getUAVData().armed;
  };
  // Check pose command works
  publishEvent("Takeoff");
  ASSERT_TRUE(test_utils::waitUntilTrue()(armed_true_fun,
                                          std::chrono::seconds(timeout_wait)));
  ASSERT_EQ(uav_system_handler_->getUAVData().localpos.z, 0.5);
  PositionYaw pose_command(1, 2, 3, 0);
  publishPoseCommand(pose_command);
  ASSERT_TRUE(test_utils::waitUntilTrue()(
      [=]() {
        ros::spinOnce();
        return (pose_command -
                getPositionYaw(uav_system_handler_->getUAVData()))
                   .position()
                   .norm() < 0.1;
      },
      std::chrono::seconds(timeout_wait)));
}

TEST_F(UAVVisionSystemHandlerTests, ReceiveStatus) {
  while (!isStatusConnected())
    ;
  ASSERT_FALSE(test_utils::waitUntilFalse()(
      [=]() {
        ros::spinOnce();
        return status_.empty();
      },
      std::chrono::seconds(timeout_wait)));
}

/// \todo Matt Add visual servoing tests

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "uav_system_handler_tests");
  return RUN_ALL_TESTS();
}
