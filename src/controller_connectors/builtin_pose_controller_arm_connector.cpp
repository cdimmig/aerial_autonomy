#include "aerial_autonomy/controller_connectors/builtin_pose_controller_arm_connector.h"
#include "aerial_autonomy/common/conversions.h"
#include <tf_conversions/tf_eigen.h>

bool BuiltInPoseControllerArmConnector::extractSensorData(
    tf::Transform &sensor_data) {
  conversions::transformMatrix4dToTf(arm_hardware_.getEndEffectorTransform(),
                                     sensor_data);

  return true;
}

void BuiltInPoseControllerArmConnector::sendControllerCommands(
    tf::Transform pose) {
  /*Eigen::Affine3d pose_eig;
  tf::transformTFToEigen(pose, pose_eig);
  if (!arm_hardware_.setEndEffectorPose(pose_eig.matrix())) {
    LOG_EVERY_N(WARNING, 50) << "End effector not in workspace";
  }
  */
}

void BuiltInPoseControllerArmConnector::initialize() {
  tf::Transform goal_pose = getGoal();
  Eigen::Affine3d pose_eig;
  tf::transformTFToEigen(goal_pose, pose_eig);
  arm_hardware_.setEndEffectorPose(pose_eig.matrix());
}
