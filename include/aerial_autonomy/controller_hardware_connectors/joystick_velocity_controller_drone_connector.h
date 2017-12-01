#pragma once
#include "aerial_autonomy/controller_hardware_connectors/base_controller_hardware_connector.h"
#include "aerial_autonomy/sensors/velocity_sensor.h"
#include "aerial_autonomy/types/empty_goal.h"
#include "aerial_autonomy/types/joystick.h"
#include "aerial_autonomy/types/position.h"
#include "aerial_autonomy/types/roll_pitch_yawrate_thrust.h"
#include "aerial_autonomy/types/velocity_yaw_rate.h"
#include <parsernode/parser.h>

/**
* @brief Maps Joystick goals to velocity goals,
* used by controller to give rpythrust commands to quadrotor
*/
class JoystickVelocityControllerDroneConnector
    : public ControllerHardwareConnector<
          std::tuple<Joystick, VelocityYawRate, double>, EmptyGoal,
          RollPitchYawRateThrust> {
public:
  /**
  * @brief Constructor
  *
  * Store drone hardware with hardware type as UAV.
  * Uses parsernode::Parser::cmdrpythrust function.
  *
  * @param drone_hardware Drone hardware used to send commands
  * @param controller Joystick velocity controller
  * @param velocity_sensor External velocity sensor
  */
  JoystickVelocityControllerDroneConnector(
      parsernode::Parser &drone_hardware,
      Controller<std::tuple<Joystick, VelocityYawRate, double>, EmptyGoal,
                 RollPitchYawRateThrust> &controller,
      std::shared_ptr<Sensor<Velocity>> velocity_sensor);

protected:
  /**
  * @brief extract sensor data from drone hardware
  *
  * @param sensor_data current joystick channels, velocity and yawrate, and
  * current yaw
  *
  * @return  false if there is an error in getting data from hardware
  */
  virtual bool
  extractSensorData(std::tuple<Joystick, VelocityYawRate, double> &sensor_data);
  /**
   * @brief  Send RPYT commands to hardware
   *
   * @param controls RPYT command to send to drone
   */
  virtual void sendHardwareCommands(RollPitchYawRateThrust controls);

private:
  /**
  * @brief Quad hardware to send commands
  */
  parsernode::Parser &drone_hardware_;
  /**
  * @brief sensor for velocity data
  */
  std::shared_ptr<Sensor<Velocity>> velocity_sensor_;
};
