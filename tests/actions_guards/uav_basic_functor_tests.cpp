#include <aerial_autonomy/actions_guards/uav_states_actions.h>
#include <aerial_autonomy/tests/sample_logic_state_machine.h>
#include <aerial_autonomy/tests/test_utils.h>
#include <gtest/gtest.h>
#include <quad_simulator_parser/quad_simulator.h>
#include <typeindex>

/**
* @brief Namespace for UAV Simulator Hardware
*/
using namespace quad_simulator;

/**
* @brief Namespace for basic states and actions for
* UAVLogicStateMachine
*/
using bsa = UAVStatesActions<UAVLogicStateMachine>;

/**
* @brief Namespace for basic events such as takeoff, land.
*/
namespace be = uav_basic_events;

// Landed
using LandedInternalActionFunctor =
    LandedInternalActionFunctor_<UAVLogicStateMachine>;
// Land
using LandInternalActionFunctor =
    LandInternalActionFunctor_<UAVLogicStateMachine>;
// Hovering
using HoveringInternalActionFunctor =
    HoveringInternalActionFunctor_<UAVLogicStateMachine>;
// Takeoff
using TakeoffInternalActionFunctor =
    TakeoffInternalActionFunctor_<UAVLogicStateMachine>;
// PositionControl
using PositionControlInternalActionFunctor =
    PositionControlInternalActionFunctor_<UAVLogicStateMachine>;
// VelocityControl
using VelocityControlInternalActionFunctor =
    VelocityControlInternalActionFunctor_<UAVLogicStateMachine>;
// ManualControl
using ManualControlInternalActionFunctor =
    ManualControlInternalActionFunctor_<UAVLogicStateMachine>;

class UAVBasicFunctorTests : public ::testing::Test {
protected:
  std::shared_ptr<QuadSimulator> drone_hardware;
  UAVSystemConfig config;
  BaseStateMachineConfig state_machine_config;
  std::unique_ptr<UAVSystem> uav_system;
  std::unique_ptr<UAVLogicStateMachine> sample_logic_state_machine;
  UAVBasicFunctorTests() {
    drone_hardware.reset(new QuadSimulator);

    // Position controller config
    auto position_controller_config =
        config.mutable_rpyt_based_position_controller_config()
            ->mutable_velocity_based_position_controller_config()
            ->mutable_position_controller_config();
    position_controller_config->mutable_goal_position_tolerance()->set_x(0.1);
    position_controller_config->mutable_goal_position_tolerance()->set_y(0.1);
    position_controller_config->mutable_goal_position_tolerance()->set_z(0.1);
    position_controller_config->set_goal_yaw_tolerance(0.1);
    auto rpyt_vel_controller_tol =
        config.mutable_rpyt_based_position_controller_config()
            ->mutable_rpyt_based_velocity_controller_config()
            ->mutable_velocity_controller_config()
            ->mutable_goal_velocity_tolerance();
    rpyt_vel_controller_tol->set_vx(0.1);
    rpyt_vel_controller_tol->set_vy(0.1);
    rpyt_vel_controller_tol->set_vz(0.1);

    // Velocity Controller config
    config.set_max_goal_velocity(2.0);
    auto velocity_tolerance = config.mutable_velocity_controller_config()
                                  ->mutable_goal_velocity_tolerance();
    velocity_tolerance->set_vx(0.1);
    velocity_tolerance->set_vy(0.1);
    velocity_tolerance->set_vz(0.1);

    // Fill quad mpc config
    auto mpc_config = config.mutable_quad_mpc_controller_config();
    test_utils::fillQuadMPCConfig(*mpc_config);
    // Fill MPC Config
    test_utils::fillMPCConfig(config);
    uav_system.reset(new UAVSystem(
        config, std::dynamic_pointer_cast<parsernode::Parser>(drone_hardware)));
    sample_logic_state_machine.reset(
        new UAVLogicStateMachine(*uav_system, state_machine_config));
  }

  virtual ~UAVBasicFunctorTests(){};
};

class LandFunctorTests : public UAVBasicFunctorTests {};

class HoveringFunctorTests : public UAVBasicFunctorTests {};

class TakeoffFunctorTests : public UAVBasicFunctorTests {};

class PositionControlFunctorTests : public UAVBasicFunctorTests {};

class ManualControlFunctorTests : public UAVBasicFunctorTests {};

class VelocityControlFunctorTests : public UAVBasicFunctorTests {};

/// \brief Test LandFunctor
TEST_F(LandFunctorTests, Constructor) {
  ASSERT_NO_THROW(new bsa::LandingAction());
  ASSERT_NO_THROW(new LandInternalActionFunctor());
}

TEST_F(LandFunctorTests, CallOperatorFunction) {
  bsa::LandingAction land_transition_action_functor;
  int dummy_start_state, dummy_target_state;
  land_transition_action_functor(be::Land(), *sample_logic_state_machine,
                                 dummy_start_state, dummy_target_state);
  ASSERT_STREQ(uav_system->getUAVData().quadstate.c_str(), "ENABLE_CONTROL ");
  // Internal Action
  LandInternalActionFunctor land_internal_action_functor;
  // Taking off which sets altitude to 0.5
  drone_hardware->takeoff();
  ASSERT_TRUE(land_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state));
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
  // After landing which sets altitude to 0.0
  drone_hardware->land();
  ASSERT_FALSE(land_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state));
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
}

TEST_F(LandFunctorTests, ManualControlInternalActionTest) {
  int dummy_event, dummy_start_state, dummy_target_state;
  drone_hardware->flowControl(false);
  LandInternalActionFunctor land_internal_action_functor;
  ASSERT_FALSE(
      land_internal_action_functor(dummy_event, *sample_logic_state_machine,
                                   dummy_start_state, dummy_target_state));
  // Check the same for landed functor
  LandedInternalActionFunctor landed_internal_action_functor;
  ASSERT_FALSE(
      landed_internal_action_functor(dummy_event, *sample_logic_state_machine,
                                     dummy_start_state, dummy_target_state));
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(ManualControlEvent)));
}
///

/// \brief Test HoveringFunctor
TEST_F(HoveringFunctorTests, Constructor) {
  ASSERT_NO_THROW(new HoveringInternalActionFunctor());
}

TEST_F(HoveringFunctorTests, CallOperatorFunction) {
  int dummy_start_state, dummy_target_state;
  // Internal Action
  HoveringInternalActionFunctor hovering_internal_action_functor;
  drone_hardware->setBatteryPercent(60);
  hovering_internal_action_functor(InternalTransitionEvent(),
                                   *sample_logic_state_machine,
                                   dummy_start_state, dummy_target_state);
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Abort)));
  // Should not land if battery is low
  drone_hardware->setBatteryPercent(20);
  hovering_internal_action_functor(InternalTransitionEvent(),
                                   *sample_logic_state_machine,
                                   dummy_start_state, dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Abort)));
}
TEST_F(HoveringFunctorTests, ManualControlInternalActionTest) {
  int dummy_event, dummy_start_state, dummy_target_state;
  drone_hardware->flowControl(false);
  HoveringInternalActionFunctor hovering_internal_action_functor;
  hovering_internal_action_functor(dummy_event, *sample_logic_state_machine,
                                   dummy_start_state, dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(ManualControlEvent)));
}
///

/// \brief Test Takeoff Functors
TEST_F(TakeoffFunctorTests, Constructor) {
  ASSERT_NO_THROW(new TakeoffInternalActionFunctor());
  ASSERT_NO_THROW(new bsa::TakeoffAction());
  ASSERT_NO_THROW(new bsa::TakeoffGuard());
  ASSERT_NO_THROW(new bsa::TakeoffAbort());
}

TEST_F(TakeoffFunctorTests, TransitionActionTest) {
  int dummy_start_state, dummy_target_state;
  bsa::TakeoffAction takeoff_transition_action_functor;
  takeoff_transition_action_functor(be::Takeoff(), *sample_logic_state_machine,
                                    dummy_start_state, dummy_target_state);
  ASSERT_STREQ(uav_system->getUAVData().quadstate.c_str(),
               "ARMED ENABLE_CONTROL ");
}

TEST_F(TakeoffFunctorTests, AbortActionTest) {
  int dummy_start_state, dummy_target_state;
  bsa::TakeoffAbort takeoff_abort_action_functor;
  takeoff_abort_action_functor(be::Abort(), *sample_logic_state_machine,
                               dummy_start_state, dummy_target_state);
  ASSERT_STREQ(uav_system->getUAVData().quadstate.c_str(), "ENABLE_CONTROL ");
}

TEST_F(TakeoffFunctorTests, TransitionGuardTest) {
  int dummy_start_state, dummy_target_state;
  bsa::TakeoffGuard takeoff_transition_guard_functor;
  drone_hardware->setBatteryPercent(60);
  bool result = takeoff_transition_guard_functor(
      be::Takeoff(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_TRUE(result);
  drone_hardware->setBatteryPercent(10);
  result = takeoff_transition_guard_functor(
      be::Takeoff(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_FALSE(result);
}

TEST_F(TakeoffFunctorTests, InternalActionTest) {
  int dummy_start_state, dummy_target_state;
  TakeoffInternalActionFunctor takeoff_internal_action_functor;
  takeoff_internal_action_functor(InternalTransitionEvent(),
                                  *sample_logic_state_machine,
                                  dummy_start_state, dummy_target_state);
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
  // After setting correct altitude
  drone_hardware->takeoff();
  takeoff_internal_action_functor(InternalTransitionEvent(),
                                  *sample_logic_state_machine,
                                  dummy_start_state, dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
}
///

/// \brief Test Takeoff Functors
TEST_F(PositionControlFunctorTests, Constructor) {
  ASSERT_NO_THROW(new PositionControlInternalActionFunctor());
  ASSERT_NO_THROW(new bsa::ReachingGoalSet());
  ASSERT_NO_THROW(new bsa::ReachingGoalGuard());
  ASSERT_NO_THROW(new bsa::UAVControllerAbort());
}

TEST_F(PositionControlFunctorTests, TransitionActionTest) {
  drone_hardware->takeoff();
  int dummy_start_state, dummy_target_state;
  bsa::ReachingGoalSet position_control_transition_action_functor;
  PositionYaw goal(1, 1, 1, 1);
  position_control_transition_action_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_EQ(
      (uav_system->getStatus<
          RPYTBasedReferenceConnector<Eigen::VectorXd, Eigen::VectorXd>>()),
      ControllerStatus::Active);
  auto resulting_goal = uav_system->getGoal<
      RPYTBasedReferenceConnector<Eigen::VectorXd, Eigen::VectorXd>,
      ReferenceTrajectoryPtr<Eigen::VectorXd, Eigen::VectorXd>>();
  auto goal_returned = resulting_goal->goal(0);
  PositionYaw resulting_position_yaw(goal_returned(0), goal_returned(1),
                                     goal_returned(2), goal_returned(5));
  ASSERT_EQ(goal, resulting_position_yaw);
}

TEST_F(PositionControlFunctorTests, AbortActionTest) {
  int dummy_start_state, dummy_target_state;
  bsa::UAVControllerAbort position_control_abort_action_functor;
  PositionYaw goal(1, 1, 1, 1);
  uav_system->setGoal<PositionControllerDroneConnector>(goal);
  position_control_abort_action_functor(be::Abort(),
                                        *sample_logic_state_machine,
                                        dummy_start_state, dummy_target_state);
  // Since the controller is aborted, will not run the controller
  uav_system->runActiveController(ControllerGroup::UAV);
  parsernode::common::quaddata data = uav_system->getUAVData();
  PositionYaw data_position_yaw(data.localpos.x, data.localpos.y,
                                data.localpos.z, data.rpydata.z);
  ASSERT_NE(data_position_yaw, goal);
}

TEST_F(PositionControlFunctorTests, TransitionGuardTest) {
  int dummy_start_state, dummy_target_state;
  PositionYaw goal(1, 1, 1, 1);
  bsa::ReachingGoalGuard position_control_transition_guard_functor;
  bool result = position_control_transition_guard_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_TRUE(result);
  goal = PositionYaw(1, 1, 1000, 1);
  result = position_control_transition_guard_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_FALSE(result);
}

TEST_F(PositionControlFunctorTests, PositionControlInternalActionTest) {
  drone_hardware->takeoff();

  int dummy_start_state, dummy_target_state;
  PositionControlInternalActionFunctor position_control_internal_action_functor;
  PositionYaw goal(1, 1, 1, 1);
  uav_system
      ->setGoal<RPYTBasedReferenceConnector<Eigen::VectorXd, Eigen::VectorXd>>(
          conversions::createWaypoint(goal));
  position_control_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));

  // Move quad to goal
  geometry_msgs::Vector3 desired_position;
  desired_position.x = goal.x;
  desired_position.y = goal.y;
  desired_position.z = goal.z;
  drone_hardware->cmdwaypoint(desired_position, goal.yaw);

  // Update controller status
  uav_system->runActiveController(ControllerGroup::UAV);

  position_control_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
}
TEST_F(PositionControlFunctorTests, ManualControlInternalActionTest) {
  int dummy_event, dummy_start_state, dummy_target_state;
  drone_hardware->flowControl(false);
  PositionControlInternalActionFunctor position_control_internal_action_functor;
  position_control_internal_action_functor(
      dummy_event, *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Abort)));
}
///
/// \brief Test Manual control functors
TEST_F(ManualControlFunctorTests, Constructor) {
  ASSERT_NO_THROW(new bsa::ManualControlSwitchAction());
  ASSERT_NO_THROW(new bsa::ManualControlSwitchGuard());
  ASSERT_NO_THROW(new ManualControlInternalActionFunctor());
}
TEST_F(ManualControlFunctorTests, ManualControlAction) {
  // Disable SDK
  drone_hardware->flowControl(false);
  // Check status in quad data is updated
  parsernode::common::quaddata data = uav_system->getUAVData();
  ASSERT_FALSE(data.rc_sdk_control_switch);
  bsa::ManualControlSwitchAction action;
  // Call action
  int dummy_event, dummy_start_state, dummy_target_state;
  action(dummy_event, *sample_logic_state_machine, dummy_start_state,
         dummy_target_state);
  // Update data and check status changed
  data = uav_system->getUAVData();
  ASSERT_TRUE(data.rc_sdk_control_switch);
}
TEST_F(ManualControlFunctorTests, ManualControlGuard) {
  // Disable SDK
  drone_hardware->flowControl(false);
  bsa::ManualControlSwitchGuard guard;
  int dummy_event, dummy_start_state, dummy_target_state;
  // Check guard result
  ASSERT_FALSE(guard(dummy_event, *sample_logic_state_machine,
                     dummy_start_state, dummy_target_state));
  // Enable SDK
  drone_hardware->flowControl(true);
  // Check guard result
  ASSERT_TRUE(guard(dummy_event, *sample_logic_state_machine, dummy_start_state,
                    dummy_target_state));
  // Set low battery
  drone_hardware->setBatteryPercent(20);
  // Check guard result
  ASSERT_FALSE(guard(dummy_event, *sample_logic_state_machine,
                     dummy_start_state, dummy_target_state));
}
TEST_F(ManualControlFunctorTests, LeaveManualMode) {
  // Takeoff
  uav_system->takeOff();
  // Call internal action
  int dummy_event, dummy_start_state, dummy_target_state;
  ManualControlInternalActionFunctor action;
  action(dummy_event, *sample_logic_state_machine, dummy_start_state,
         dummy_target_state);
  // Check Takeoff event is triggered
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Takeoff)));
  // Land
  uav_system->land();
  // Call action again
  action(dummy_event, *sample_logic_state_machine, dummy_start_state,
         dummy_target_state);
  // Check Land event is triggered
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Land)));
}
///
/// \brief Velocity Control tests
TEST_F(VelocityControlFunctorTests, Constructor) {
  ASSERT_NO_THROW(new VelocityControlInternalActionFunctor());
  ASSERT_NO_THROW(new bsa::SetVelocityGoal());
  ASSERT_NO_THROW(new bsa::GuardVelocityGoal());
}

TEST_F(VelocityControlFunctorTests, TransitionActionTest) {
  int dummy_start_state, dummy_target_state;
  bsa::SetVelocityGoal velocity_control_transition_action_functor;
  VelocityYaw goal(1, 1, 1, 1);
  velocity_control_transition_action_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_EQ(uav_system->getStatus<BuiltInVelocityControllerDroneConnector>(),
            ControllerStatus::Active);
  VelocityYaw resulting_goal =
      uav_system
          ->getGoal<BuiltInVelocityControllerDroneConnector, VelocityYaw>();
  ASSERT_EQ(goal, resulting_goal);
  uav_system->runActiveController(ControllerGroup::UAV);
  ASSERT_EQ(uav_system->getStatus<BuiltInVelocityControllerDroneConnector>(),
            ControllerStatus::Active);
  parsernode::common::quaddata data = uav_system->getUAVData();
  VelocityYaw data_velocity_yaw(data.linvel.x, data.linvel.y, data.linvel.z,
                                data.rpydata.z);
  ASSERT_EQ(data_velocity_yaw, goal);
  uav_system->runActiveController(ControllerGroup::UAV);
  ASSERT_EQ(uav_system->getStatus<BuiltInVelocityControllerDroneConnector>(),
            ControllerStatus::Completed);
}

TEST_F(VelocityControlFunctorTests, AbortActionTest) {
  int dummy_start_state, dummy_target_state;
  bsa::UAVControllerAbort uav_control_abort_action_functor;
  VelocityYaw goal(1, 1, 1, 1);
  uav_system->setGoal<BuiltInVelocityControllerDroneConnector>(goal);
  uav_control_abort_action_functor(be::Abort(), *sample_logic_state_machine,
                                   dummy_start_state, dummy_target_state);
  ASSERT_EQ(uav_system->getActiveControllerStatus(ControllerGroup::UAV),
            ControllerStatus::NotEngaged);
  // Since the controller is aborted, will not run the controller
  uav_system->runActiveController(ControllerGroup::UAV);
  parsernode::common::quaddata data = uav_system->getUAVData();
  VelocityYaw data_position_yaw(data.linvel.x, data.linvel.y, data.linvel.z,
                                data.rpydata.z);
  ASSERT_NE(data_position_yaw, goal);
}

TEST_F(VelocityControlFunctorTests, TransitionGuardTest) {
  int dummy_start_state, dummy_target_state;
  VelocityYaw goal(1, 1, 1, 1);
  bsa::GuardVelocityGoal velocity_control_transition_guard_functor;
  bool result = velocity_control_transition_guard_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_TRUE(result);
  goal = VelocityYaw(1, 1, 2.1, 1);
  result = velocity_control_transition_guard_functor(
      goal, *sample_logic_state_machine, dummy_start_state, dummy_target_state);
  ASSERT_FALSE(result);
}

TEST_F(VelocityControlFunctorTests, InternalActionTest) {
  drone_hardware->takeoff();
  int dummy_start_state, dummy_target_state;
  VelocityControlInternalActionFunctor velocity_control_internal_action_functor;
  VelocityYaw goal(1, 1, 1, 1);
  uav_system->setGoal<BuiltInVelocityControllerDroneConnector>(goal);
  velocity_control_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
  // After running the active controller once updates quad state
  uav_system->runActiveController(ControllerGroup::UAV);
  // Second time updates controller status
  uav_system->runActiveController(ControllerGroup::UAV);
  velocity_control_internal_action_functor(
      InternalTransitionEvent(), *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  // Even when the controller is completed, the state will not abort
  ASSERT_NE(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(Completed)));
}
TEST_F(VelocityControlFunctorTests, ManualControlInternalActionTest) {
  int dummy_event, dummy_start_state, dummy_target_state;
  drone_hardware->flowControl(false);
  VelocityControlInternalActionFunctor velocity_control_internal_action_functor;
  velocity_control_internal_action_functor(
      dummy_event, *sample_logic_state_machine, dummy_start_state,
      dummy_target_state);
  ASSERT_EQ(sample_logic_state_machine->getProcessEventTypeId(),
            std::type_index(typeid(be::Abort)));
}
///

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
