/// @file
///
/// kuka_control is designed to compute the torque command based on
/// desired joint position, velocity, and acceleration, and measured joint position and velocity.
/// Currently, the final torque command is composed of inverse dynamics torque command and joint position PD
/// controller command.
/// (TODO: Generalize this ID controller to more general types of feedback controllers)
/// Messages are sent via LCM channels.

#include <lcm/lcm-cpp.hpp>
#include <ctime>

#include <cmath>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <string>
#include <list>
#include <iostream>

#include "drake/common/drake_assert.h"
#include "drake/common/drake_path.h"
#include "drake/examples/kuka_iiwa_arm/iiwa_common.h"
#include "drake/multibody/parsers/urdf_parser.h"
#include "drake/multibody/rigid_body_tree.h"

#include "drake/lcmt_robot_controller_reference.hpp"
#include "drake/lcmt_iiwa_status.hpp"
#include "drake/lcmt_iiwa_command.hpp"
#include "drake/lcmt_polynomial.hpp" // temporarily abuse one lcm channel
#define KUKA_DATA_DIR "/home/yezhao/kuka-dev-estimation/drake/drake/examples/kuka_iiwa_arm/experiment_data/torque_command_analysis/"

#include "drake/util/drakeGeometryUtil.h"

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::VectorXi;
using drake::Vector1d;
using Eigen::Vector2d;
using Eigen::Vector3d;

static std::list< const char*> gs_fileName;
static std::list< std::string > gs_fileName_string;

namespace drake {
namespace examples {
namespace kuka_iiwa_arm {
namespace {

const char* const kLcmStatusChannel = "IIWA_STATUS";
const char* const kLcmControlRefChannel = "CONTROLLER_REFERENCE";
const char* const kLcmCommandChannel = "IIWA_COMMAND";
const char* const kLcmParamChannel = "IIWA_PARAM";
const char* const kCancelPlanRunning = "CANCEL_PLAN";


const int kNumJoints = 7;
const double joint_velocity_threthold = 0.05;
const double joint_torque_threthold = 0.1;

// 1: ID controller type 1, qddot_feedforward = PD position control + desired qddot, then send this qddot_feedforward to ID
// 2: ID controller type 2, feedforward ID (purly based on desired qddot) + PD impedance control
const int inverseDynamicsCtrlType = 2;

class RobotController {
 public:
   bool run_ = false;

  /// tree is aliased
  explicit RobotController(const RigidBodyTree<double>& tree)
      : tree_(tree), controller_trigger_(false) {
    VerifyIiwaTree(tree);
    lcm_.subscribe(kLcmStatusChannel,
                    &RobotController::HandleStatus, this);
    lcm_.subscribe(kLcmControlRefChannel,
                    &RobotController::HandleControl, this);
    lcm_.subscribe(kCancelPlanRunning,
                    &RobotController::HandleCancelPlan, this);
  }

  void Run() {
    int64_t cur_time_us = -1;

    // Initialize the timestamp to an invalid number so we can detect the first message.
    iiwa_status_.utime = cur_time_us;
    robot_controller_reference_.utime = cur_time_us;

    lcmt_iiwa_command iiwa_command;
    iiwa_command.num_joints = kNumJoints;
    iiwa_command.joint_position.resize(kNumJoints, 0.);
    iiwa_command.num_torques = kNumJoints;
    iiwa_command.joint_torque.resize(kNumJoints, 0.);

    lcmt_polynomial iiwa_param;
    iiwa_param.num_coefficients = kNumJoints;
    iiwa_param.coefficients.resize(kNumJoints, 0.);


    Eigen::VectorXd joint_position_desired(kNumJoints);
    Eigen::VectorXd joint_velocity_desired(kNumJoints);
    Eigen::VectorXd joint_accel_desired(kNumJoints);

    bool half_servo_rate_flag_ = false; // make the iiwa command get published every two servo loops

    // friction model coefficients
    Eigen::VectorXd viscous_coeff(kNumJoints);
    Eigen::VectorXd Coulomb_coeff(kNumJoints);
    viscous_coeff << 0.1915, 8.45, 0.0108, 1.4523, 0.0178, 0.1199, 0.05;
    Coulomb_coeff << 1.7809, 1.9103, 0.0445, 0.2538, 0.1151, 0.0534, 0.4934;

    while (true) {

      // Call lcm handle until at least one message is processed
      while (0 == lcm_.handleTimeout(10)) { }

      if (!run_){
        // std::cout << "I am quitting!  \n" << run_ << std::endl;
        iiwa_status_.utime = iiwa_status_.utime;

        lcmt_iiwa_command iiwa_command;
        iiwa_command.num_joints = kNumJoints;
        iiwa_command.joint_position.resize(kNumJoints, 0.);
        iiwa_command.num_torques = kNumJoints;
        iiwa_command.joint_torque.resize(kNumJoints, 0.);
      }

      DRAKE_ASSERT(iiwa_status_.utime != -1);
      cur_time_us = iiwa_status_.utime;

      if (controller_trigger_) {
        const int kNumDof = 7;
        iiwa_command.utime = iiwa_status_.utime;
        iiwa_param.timestamp = iiwa_status_.utime;

        // Set desired joint position, velocity and acceleration
        for (int joint = 0; joint < kNumDof; joint++){
          joint_position_desired(joint) = robot_controller_reference_.joint_position_desired[joint];
          joint_velocity_desired(joint) = robot_controller_reference_.joint_velocity_desired[joint];
          joint_accel_desired(joint) = robot_controller_reference_.joint_accel_desired[joint];
        }

        double *qptr = &iiwa_status_.joint_position_measured[0];
        Eigen::Map<Eigen::VectorXd> q(qptr, kNumDof);
        double *qdptr = &iiwa_status_.joint_velocity_estimated[0];
        Eigen::Map<Eigen::VectorXd> qd(qdptr, kNumDof);

        Eigen::VectorXd torque_command(kNumDof);
        Eigen::VectorXd position_ctrl_torque_command(kNumDof);
        Eigen::VectorXd gravity_torque(kNumDof);
        // Inverse dynamics Controller, first choose controller type.
        if (inverseDynamicsCtrlType == 1){
          // ------- torque control version 1: qddot_des + PD impedance control --> qddot_ff --> inverse dynamics

          // PD position control
          Eigen::VectorXd Kp_pos_ctrl(kNumDof); // 7 joints
          Kp_pos_ctrl << 160, 200, 70, 60, 45, 20, 10;// Mitchell's gains for GPS
          Eigen::VectorXd Kd_pos_ctrl(kNumDof); // 7 joints
          Kd_pos_ctrl << 20, 33, 20, 15, 3, 5, 1; // Mitchell's gains for GPS
          // (TODOs) Add integral control (anti-windup)

          // Set desired joint position, velocity and acceleration
          for (int joint = 0; joint < kNumDof; joint++){
            joint_accel_desired(joint) += Kp_pos_ctrl(joint)*(joint_position_desired(joint) - iiwa_status_.joint_position_measured[joint])
                                                + Kd_pos_ctrl(joint)*(joint_velocity_desired(joint) - iiwa_status_.joint_velocity_estimated[joint]);
          }

          // Computing inverse dynamics torque command
          KinematicsCache<double> cache = tree_.doKinematics(q, qd);
          const RigidBodyTree<double>::BodyToWrenchMap no_external_wrenches;
          torque_command = tree_.inverseDynamics(cache, no_external_wrenches, joint_accel_desired, false);

          // add Coulomb-viscous friction model to joint torque, there could be a better way to incorporate these friction parameters into URDF files
          for(int joint = 0; joint < kNumJoints; joint++){
            if (fabs(iiwa_status_.joint_velocity_estimated[joint]) < joint_velocity_threthold && fabs(torque_command(joint)) > joint_torque_threthold)
              torque_command(joint) += Coulomb_coeff(joint)*torque_command(joint)/fabs(torque_command(joint));
            else if (fabs(iiwa_status_.joint_velocity_estimated[joint]) > joint_velocity_threthold)
              torque_command(joint) += viscous_coeff(joint)*iiwa_status_.joint_velocity_estimated[joint];
          }

          // gravity compensation without gripper (to cancel out the low-level kuka controller)
          Eigen::VectorXd z = Eigen::VectorXd::Zero(kNumDof);
          gravity_torque = gravity_comp_no_gripper(cache, z, false, tree_);
          torque_command -= gravity_torque;

        }else if (inverseDynamicsCtrlType == 2){
          // ------- torque control version 2: feedforward inverse dynamics + PD impedance control
          // Computing inverse dynamics torque command
          KinematicsCache<double> cache = tree_.doKinematics(q, qd);
          const RigidBodyTree<double>::BodyToWrenchMap no_external_wrenches;
          torque_command = tree_.inverseDynamics(cache, no_external_wrenches, joint_accel_desired, false);

          //debugging for feedforward and feedback torque components
          /*if (fabs(iiwa_status_.joint_velocity_estimated[5]) > 0.1 || fabs(iiwa_status_.joint_velocity_estimated[3]) > 0.1 || fabs(iiwa_status_.joint_velocity_estimated[1]) > 0.1)
            saveVector(torque_command, "feedforward_inverse_dynamics_command");*/

          // PD position control
          Eigen::VectorXd Kp_pos_ctrl(kNumDof); // 7 joints
          //Kp_pos_ctrl << 225, 361, 144, 150, 100, 20, 20;// very large gains after system id
          // Kp_pos_ctrl << 225, 361, 144, 81, 80, 36, 20;// best gains (in terms of position accuracy) after system id harvard
          Kp_pos_ctrl << 225, 361, 154, 81, 89, 36, 20;// best gains (in terms of position accuracy) after system id draper

          //Kp_pos_ctrl << 120, 120, 60, 60, 60, 30, 20;// medium gains
          //Kp_pos_ctrl << 80, 80, 30, 30, 20, 20, 10;// reduce the gains as much as possible while maintaining the position tracking performance
          Eigen::VectorXd Kd_pos_ctrl(kNumDof); // 7 joints
          //Kd_pos_ctrl << 30, 35, 14, 15, 10, 3, 3;// very large gains after system id
          // Kd_pos_ctrl << 25, 33, 20, 15, 3, 2, 3;// best gains (in terms of position accuracy) after system id harvard
          Kd_pos_ctrl << 25, 33, 18, 13, 2, 2, 3;// best gains (in terms of position accuracy) after system id draper

          //Kd_pos_ctrl << 15, 15, 6, 6, 6, 4, 4;// medium gains after system id
          //Kd_pos_ctrl << 10, 10, 3, 3, 3, 2, 3;// reduce the gains as much as possible while maintaining the position tracking performance
          // (TODOs) Add integral control (anti-windup)
          for (int joint = 0; joint < kNumJoints; joint++) {
            position_ctrl_torque_command(joint) = Kp_pos_ctrl(joint)*(joint_position_desired(joint) - iiwa_status_.joint_position_measured[joint])
                                                + Kd_pos_ctrl(joint)*(joint_velocity_desired(joint) - iiwa_status_.joint_velocity_estimated[joint]);
            // position_ctrl_torque_command(joint) = 0.0;
          }
          //Combination of ID torque control and IK position control
          torque_command += position_ctrl_torque_command;

          // gravity compensation without gripper (to cancel out the low-level kuka controller)
          Eigen::VectorXd z = Eigen::VectorXd::Zero(kNumDof);
          gravity_torque = gravity_comp_no_gripper(cache, z, false, tree_);
          torque_command -= gravity_torque;

        }
        std::cout << "TORQUE:" << torque_command << std::endl;
        //debugging for feedforward and feedback torque components
        /*if (fabs(iiwa_status_.joint_velocity_estimated[5]) > 0.1 || fabs(iiwa_status_.joint_velocity_estimated[3]) > 0.1 || fabs(iiwa_status_.joint_velocity_estimated[1]) > 0.1){
          saveVector(position_ctrl_torque_command, "PD_impedance_ctrl_command");
          saveVector(torque_command+gravity_torque, "total_torque_command");
        }*/

        // -------->(For Safety) Set up iiwa position command<----------
        for (int joint = 0; joint < kNumJoints; joint++) {
          iiwa_command.joint_position[joint] = joint_position_desired(joint);
        }

        // -------->Set up iiwa torque command<-------------
        for (int joint = 0; joint < kNumJoints; joint++) {
          iiwa_command.joint_torque[joint] = torque_command(joint);
          iiwa_command.joint_torque[joint] = std::max(-150.0, std::min(150.0, iiwa_command.joint_torque[joint]));
          iiwa_param.coefficients[joint] = robot_controller_reference_.joint_position_desired[joint];
        }

        if (half_servo_rate_flag_ && controller_trigger_){
          half_servo_rate_flag_ = false;
          lcm_.publish(kLcmCommandChannel, &iiwa_command);
          lcm_.publish(kLcmParamChannel, &iiwa_param);

        }else{
          half_servo_rate_flag_ = true;
        }
      }
    }
  }

  void saveVector(const Eigen::VectorXd & _vec, const char * _name){
      std::string _file_name = KUKA_DATA_DIR;
      _file_name += _name;
      _file_name += ".dat";
      clean_file(_name, _file_name);

      std::ofstream save_file;
      save_file.open(_file_name, std::fstream::app);
      for (int i(0); i < _vec.rows(); ++i){
          save_file<<_vec(i)<< "\t";
      }
      save_file<<"\n";
      save_file.flush();
      save_file.close();
  }

  void saveValue(double _value, const char * _name){
      std::string _file_name = KUKA_DATA_DIR;
      _file_name += _name;
      _file_name += ".dat";
      clean_file(_name, _file_name);

      std::ofstream save_file;
      save_file.open(_file_name, std::fstream::app);
      save_file<<_value <<"\n";
      save_file.flush();
  }

  void clean_file(const char * _file_name, std::string & _ret_file){
      std::list<std::string>::iterator iter = std::find(gs_fileName_string.begin(), gs_fileName_string.end(), _file_name);
      if(gs_fileName_string.end() == iter){
          gs_fileName_string.push_back(_file_name);
          remove(_ret_file.c_str());
      }
  }

 private:
  void HandleStatus(const lcm::ReceiveBuffer* rbuf, const std::string& chan,
                    const lcmt_iiwa_status* status) {
    iiwa_status_ = *status;
  }
  void HandleCancelPlan(const lcm::ReceiveBuffer* rbuf, const std::string& chan,
                    const lcmt_iiwa_status* status) {
    std::cout << "Plan Cancel Command Recieved!" << std::endl;
    run_ = false;
    controller_trigger_ = false;

  }
  void HandleControl(const lcm::ReceiveBuffer* rbuf, const std::string& chan,
                    const lcmt_robot_controller_reference* input) {
    robot_controller_reference_ = *input;
    controller_trigger_ = true;
    run_ = true;

  }

  Eigen::VectorXd gravity_comp_no_gripper(KinematicsCache<double>& cache, const Eigen::VectorXd& vd,
      bool include_velocity_terms, const RigidBodyTree<double>& tree) const {
    cache.checkCachedKinematicsSettings(include_velocity_terms, include_velocity_terms, "gravity_comp_no_gripper");

    const bool include_acceleration_terms = true;
    int num_joints = 7;
    int kTwistSize = 6;
    unsigned int body_size_no_gripper = tree.FindBodyIndex("iiwa_link_ee") + 1; // the last arm link before gripper links, + 1 is due to additional world frame

    // Compute spatial accelerations and net wrenches that should be exerted to
    // achieve those accelerations.
    Matrix6X<double> body_accelerations(kTwistSize, body_size_no_gripper);
    Matrix6X<double> net_wrenches(kTwistSize, body_size_no_gripper);
    for (size_t i = 0; i < body_size_no_gripper; ++i) {
      const RigidBody<double>& body = *tree.bodies[i];
      if (body.has_parent_body()) {
        const RigidBody<double>& parent_body = *(body.get_parent());
        const auto& cache_element = cache.get_element(i);

        auto body_acceleration = body_accelerations.col(i);

        // Initialize body acceleration to acceleration of parent body.
        auto parent_acceleration =
            body_accelerations.col(parent_body.get_body_index());
        body_acceleration = parent_acceleration;
        // Add component due to joint acceleration.
        if (include_acceleration_terms) {
          const DrakeJoint& joint = body.getJoint();
          auto vd_joint = vd.middleRows(body.get_velocity_start_index(),
                                        joint.get_num_velocities());
          body_acceleration.noalias() +=
              cache_element.motion_subspace_in_world * vd_joint;
        }
        auto net_wrench = net_wrenches.col(i);
        const auto& body_inertia = cache_element.inertia_in_world;
        net_wrench.noalias() = body_inertia * body_acceleration;
      } else {
        drake::TwistVector<double> a_grav;
        a_grav << 0, 0, 0, 0, 0, -9.81;
        body_accelerations.col(i) = -a_grav.cast<double>();
        net_wrenches.col(i).setZero();
      }
    }

    // Do a backwards pass to compute joint wrenches from net wrenches,
    // and project the joint wrenches onto the joint's motion subspace to find the joint torque.
    auto& joint_wrenches = net_wrenches;
    const auto& joint_wrenches_const = net_wrenches;
    VectorX<double> gravity_torques(num_joints, 1);

    for (ptrdiff_t i = body_size_no_gripper - 1; i >= 0; --i) {
      RigidBody<double>& body = *tree.bodies[i];
      if (body.has_parent_body()) {
        const auto& cache_element = cache.get_element(i);
        const auto& joint = body.getJoint();
        auto joint_wrench = joint_wrenches_const.col(i);

        const auto& motion_subspace = cache_element.motion_subspace_in_world;
        auto joint_torques = gravity_torques.middleRows(body.get_velocity_start_index(), joint.get_num_velocities());
        joint_torques.noalias() = motion_subspace.transpose() * joint_wrench;

        const RigidBody<double>& parent_body = *(body.get_parent());
        auto parent_joint_wrench = joint_wrenches.col(parent_body.get_body_index());
        parent_joint_wrench += joint_wrench;
      }
    }

    return gravity_torques;
  }

  lcm::LCM lcm_;
  const RigidBodyTree<double>& tree_;
  bool controller_trigger_;// control runner wait for the first message from plan runner
  lcmt_iiwa_status iiwa_status_;
  lcmt_robot_controller_reference robot_controller_reference_;
};

int DoMain(int argc, const char* argv[]) {

  auto tree = std::make_unique<RigidBodyTree<double>>();
  parsers::urdf::AddModelInstanceFromUrdfFileToWorld(
    GetDrakePath() + "/examples/kuka_iiwa_arm/urdf/iiwa14_simplified_collision.urdf",
      multibody::joints::kFixed, tree.get());

  RobotController runner(*tree);
  runner.Run();
  return 0;
}

}  // namespace
}  // namespace kuka_iiwa_arm
}  // namespace examples
}  // namespace drake

int main(int argc, const char* argv[]) {
  return drake::examples::kuka_iiwa_arm::DoMain(argc, argv);
}
