/* 
 * Copyright (c) 2011, Vanadium Labs LLC
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Vanadium Labs LLC nor the names of its 
 *       contributors may be used to endorse or promote products derived 
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL VANADIUM LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Author: Michael Ferguson, Helen Oleynikova
 */

#include <ros/ros.h>
#include <tf/tf.h>
#include <actionlib/server/simple_action_server.h>
#include <turtlebot_block_manipulation/PickAndPlaceAction.h>

#include <simple_arm_server/MoveArm.h>
#include <simple_arm_server/ArmAction.h>

/*const std::string arm_link = "/arm_base_link";
const double gripper_open = 0.04;
const double gripper_closed = 0.024;

const double z_up = 0.08;
const double z_down = -0.04;
*/

class PickAndPlaceServer
{
private:
    
  ros::NodeHandle nh_;
  actionlib::SimpleActionServer<turtlebot_block_manipulation::PickAndPlaceAction> as_;
  std::string action_name_;
  
  turtlebot_block_manipulation::PickAndPlaceFeedback feedback_;
  turtlebot_block_manipulation::PickAndPlaceResult result_;
  turtlebot_block_manipulation::PickAndPlaceGoalConstPtr goal_;
  
  ros::ServiceClient client_;
  ros::Subscriber sub_;
  ros::Publisher pub_;
  
  // Parameters
  std::string arm_link;
  double gripper_open;
  double gripper_closed;
  double z_up;
  double z_down;
  
public:
  PickAndPlaceServer(const std::string name) : 
    nh_(), as_(nh_, name, false), action_name_(name)
  {
    //register the goal and feeback callbacks
    as_.registerGoalCallback(boost::bind(&PickAndPlaceServer::goalCB, this));
    as_.registerPreemptCallback(boost::bind(&PickAndPlaceServer::preemptCB, this));
    
    as_.start();
    
    client_ = nh_.serviceClient<simple_arm_server::MoveArm>("simple_arm_server/move");
  }

  void goalCB()
  {
    goal_ = as_.acceptNewGoal();
    arm_link = goal_->frame;
    gripper_open = goal_->gripper_open;
    gripper_closed = goal_->gripper_closed;
    z_up = goal_->z_up;
    z_down = goal_->z_down;
    
    pickAndPlace(goal_->pickup_pose, goal_->place_pose);
  }

  void preemptCB()
  {
    ROS_INFO("%s: Preempted", action_name_.c_str());
    // set the action state to preempted
    as_.setPreempted();
  }
  
  void pickAndPlace(const geometry_msgs::Pose& start_pose, const geometry_msgs::Pose& end_pose)
  {
    simple_arm_server::MoveArm srv;
    simple_arm_server::ArmAction action;
    simple_arm_server::ArmAction grip;
    
    /* open gripper */
    grip.type = simple_arm_server::ArmAction::MOVE_GRIPPER;
    grip.move_time.sec = 1.0;
    grip.command = gripper_open;
    srv.request.goals.push_back(grip);
    
    /* arm straight up */
    btQuaternion temp;
    temp.setRPY(0,1.57,0);
    action.goal.orientation.x = temp.getX();
    action.goal.orientation.y = temp.getY();
    action.goal.orientation.z = temp.getZ();
    action.goal.orientation.w = temp.getW();

    /* hover over */
    action.goal.position.x = start_pose.position.x;
    action.goal.position.y = start_pose.position.y;
    action.goal.position.z = z_up;
    srv.request.goals.push_back(action);
    action.move_time.sec = 1.5;

    /* go down */
    action.goal.position.z = z_down;
    srv.request.goals.push_back(action);
    action.move_time.sec = 1.5;

    /* close gripper */
    grip.type = simple_arm_server::ArmAction::MOVE_GRIPPER;
    grip.command = gripper_closed;
    grip.move_time.sec = 1.0;
    srv.request.goals.push_back(grip);

    /* go up */
    action.goal.position.z = z_up;
    srv.request.goals.push_back(action);
    action.move_time.sec = 0.25;

    /* hover over */
    action.goal.position.x = end_pose.position.x;
    action.goal.position.y = end_pose.position.y;
    action.goal.position.z = z_up;
    srv.request.goals.push_back(action);
    action.move_time.sec = 1.5;

    /* go down */
    action.goal.position.z = z_down;
    srv.request.goals.push_back(action);
    action.move_time.sec = 1.5;

    /* open gripper */
    grip.command = gripper_open;
    srv.request.goals.push_back(grip);

    /* go up */
    action.goal.position.z = z_up;
    srv.request.goals.push_back(action);
    action.move_time.sec = 0.25;
  
    srv.request.header.frame_id = arm_link;
    if (client_.call(srv))
      as_.setSucceeded(result_);
    else
      as_.setAborted(result_);
  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "pick_and_place_action_server");

  PickAndPlaceServer server("pick_and_place");
  ros::spin();

  return 0;
}

