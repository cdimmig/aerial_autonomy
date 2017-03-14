#pragma once

#include "uav_system_config.pb.h"
#include <aerial_autonomy/controller_hardware_connectors/base_controller_hardware_connector.h>
// Controllers
#include <aerial_autonomy/controllers/basic_controllers.h>
// Specific ControllerConnectors
#include <aerial_autonomy/controller_hardware_connectors/basic_controller_hardware_connectors.h>
// Store type_map
#include <aerial_autonomy/common/type_map.h>
// Boost thread stuff
#include <boost/thread/mutex.hpp>
// Unique ptr
#include <memory>

/**
 * @brief Owns, initializes, and facilitates communication between different
 * hardware/software components.
 * Provides builtin position, velocity, and rpy controllers for controlling UAV
*/
class UAVSystem {

private:
  /**
  * @brief Hardware
  */
  parsernode::Parser &drone_hardware_;
  // Controllers
  /**
  * @brief Position Controller
  */
  BuiltInController<PositionYaw> builtin_position_controller_;
  /**
  * @brief  velocity controller
  */
  BuiltInController<VelocityYaw> builtin_velocity_controller_;
  /**
  * @brief rpyt controller using joystick controller connectors
  */
  ManualRPYTController manual_rpyt_controller_;
  /**
   * @brief connector for position controller
   */
  PositionControllerDroneConnector position_controller_drone_connector_;
  /**
  * @brief connector for velocity controller
  */
  BuiltInVelocityControllerDroneConnector velocity_controller_drone_connector_;
  /**
  * @brief connector for rpyt controller
  */
  ManualRPYTControllerDroneConnector rpyt_controller_drone_connector_;
  /**
   * @brief Container to store and retrieve controller-hardware-connectors
   */
  TypeMap<AbstractControllerHardwareConnector>
      controller_hardware_connector_container_;
  /**
  * @brief Map to store active controller based on hardware type
  */
  std::map<HardwareType, AbstractControllerHardwareConnector *>
      active_controllers_;
  /**
  * @brief Map to lock and swap the active controller for a given
  * hardware type
  */
  std::map<HardwareType, std::unique_ptr<boost::mutex>> thread_mutexes_;

  /**
   * @brief UAV configuration parameters
   */
  UAVSystemConfig config_;

public:
  /**
   * @brief Constructor with default configuration
   */
  UAVSystem(parsernode::Parser &drone_hardware)
      : UAVSystem(drone_hardware, UAVSystemConfig()) {}
  /**
  * @brief Constructor
  *
  * UAVSystem requires a drone hardware. It instantiates the connectors,
  * controllers
  *
  * @param drone_hardware input hardware to send commands back
  * @param config The system configuration specifying the parameters such as
  * takeoff height, etc.
  */
  UAVSystem(parsernode::Parser &drone_hardware, UAVSystemConfig config)
      : drone_hardware_(drone_hardware),
        position_controller_drone_connector_(drone_hardware,
                                             builtin_position_controller_),
        velocity_controller_drone_connector_(drone_hardware,
                                             builtin_velocity_controller_),
        rpyt_controller_drone_connector_(drone_hardware,
                                         manual_rpyt_controller_),
        config_(config) {
    // Add control hardware connector containers
    controller_hardware_connector_container_.setObject(
        position_controller_drone_connector_);
    controller_hardware_connector_container_.setObject(
        velocity_controller_drone_connector_);
    controller_hardware_connector_container_.setObject(
        rpyt_controller_drone_connector_);

    // Initialize active controller map
    /// \todo make enum class iterable to do this automatically
    active_controllers_[HardwareType::Arm] = nullptr;
    thread_mutexes_[HardwareType::Arm] =
        std::unique_ptr<boost::mutex>(new boost::mutex);

    active_controllers_[HardwareType::UAV] = nullptr;
    thread_mutexes_[HardwareType::UAV] =
        std::unique_ptr<boost::mutex>(new boost::mutex);
  }
  /**
  * @brief Get sensor data from UAV
  *
  * @return Accumulated sensor data from UAV
  */
  parsernode::common::quaddata getUAVData() {
    parsernode::common::quaddata data;
    drone_hardware_.getquaddata(data);
    return data;
  }

  /**
  * @brief Public API call to takeoff
  */
  void takeOff() { drone_hardware_.takeoff(); }

  /**
  * @brief Public API call to land
  */
  void land() { drone_hardware_.land(); }

  /**
  * @brief sets goal to the connector and swaps the active
  * connector with the specified connector type.
  *
  * @tparam ControllerHardwareConnectorT type of connector to use
  * @tparam GoalT Type of Goal to set to connector
  * @param goal Goal to set to connector
  */
  template <class ControllerHardwareConnectorT, class GoalT>
  void setGoal(GoalT goal) {
    ControllerHardwareConnectorT *controller_hardware_connector =
        controller_hardware_connector_container_
            .getObject<ControllerHardwareConnectorT>();
    controller_hardware_connector->setGoal(goal);
    HardwareType hardware_type =
        controller_hardware_connector->getHardwareType();
    if (active_controllers_[hardware_type] != controller_hardware_connector) {
      boost::mutex::scoped_lock lock(*thread_mutexes_[hardware_type]);
      active_controllers_[hardware_type] = controller_hardware_connector;
    }
  }
  /**
  * @brief Get the goal from connector.
  *
  * @tparam ControllerHardwareConnectorT Type of connector to use
  * @tparam GoalT Type of Goal to get
  *
  * @return goal of GoalT type
  */
  template <class ControllerHardwareConnectorT, class GoalT> GoalT getGoal() {
    ControllerHardwareConnectorT *controller_hardware_connector =
        controller_hardware_connector_container_
            .getObject<ControllerHardwareConnectorT>();
    return controller_hardware_connector->getGoal();
  }

  /**
  * @brief Remove active controller for given hardware type
  *
  * @param hardware_type Type of hardware for which active controller is
  * switched off
  */
  void abortController(HardwareType hardware_type) {
    boost::mutex::scoped_lock lock(*thread_mutexes_[hardware_type]);
    active_controllers_[hardware_type] = nullptr;
  }

  /**
  * @brief Run active controller stored for a given hardware type
  *
  * @param hardware_type Type of hardware for which active controller is run
  */
  void runActiveController(HardwareType hardware_type) {
    // lock to ensure active_control fcn is not changed
    AbstractControllerHardwareConnector *active_controller =
        active_controllers_[hardware_type];
    if (active_controller != nullptr) {
      boost::mutex::scoped_lock lock(*thread_mutexes_[hardware_type]);
      active_controller->run();
    }
  }

  std::string getSystemStatus() {
    parsernode::common::quaddata data = getUAVData();
    // Get goals for different controllers:
    VelocityYaw velocity_controller_goal =
        builtin_velocity_controller_.getGoal();
    PositionYaw position_controller_goal =
        builtin_position_controller_.getGoal();
    char buffer[1500];
    sprintf(buffer,
            "Battery Percent: %2.2f\t\nlx: %2.2f\tly: %2.2f\tlz: "
            "%2.2f\nAltitude: %2.2f\t\nRoll: %2.2f\tPitch %2.2f\tYaw %2.2f\n"
            "Magx: %2.2f\tMagy %2.2f\tMagz %2.2f\naccx: %2.2f\taccy "
            "%2.2f\taccz %2.2f\nvelx: %2.2f\tvely %2.2f\tvelz %2.2f\n"
            "Goalvx: %2.2f\tGoalvy: %2.2f\tGoalvz: %2.2f\tGoalvyaw: %2.2f\n"
            "Goalx: %2.2f\tGoaly: %2.2f\tGoalz: %2.2f\tGoalpyaw: %2.2f\n"
            "Mass: %2.2f\tTimestamp: %2.2f\t\nQuadState: %s",
            data.batterypercent, data.localpos.x, data.localpos.y,
            data.localpos.z, data.altitude, data.rpydata.x * (180 / M_PI),
            data.rpydata.y * (180 / M_PI),
            data.rpydata.z * (180 / M_PI), // IMU rpy angles
            data.magdata.x, data.magdata.y, data.magdata.z, data.linacc.x,
            data.linacc.y, data.linacc.z, data.linvel.x, data.linvel.y,
            data.linvel.z, velocity_controller_goal.x,
            velocity_controller_goal.y, velocity_controller_goal.z,
            velocity_controller_goal.yaw, position_controller_goal.x,
            position_controller_goal.y, position_controller_goal.z,
            position_controller_goal.yaw, data.mass, data.timestamp,
            data.quadstate.c_str());
    return buffer;
  }

  /**
   * @brief Get system configuration
   * @return Configuration
   */
  UAVSystemConfig getConfiguration() { return config_; }
};
