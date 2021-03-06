#include "ros/ros.h"
#include "nav_msgs/Path.h"
#include "geometry_msgs/PoseStamped.h"
#include "tf/tf.h"
#include <tf/transform_listener.h>
#include "joystick2coms/VehicleMessageStamp.h"

double steering = 0.0;
double speed = 0.0;

double dis = 0;	
double kk  = 0;
double delta_x  = 0;
double delta_y  = 0;
double delta_th = 0;
double sum_dis = 0;
ros::Time ros_time_last;
ros::Time ros_time_current;

geometry_msgs::PoseStamped vb_point;
geometry_msgs::PoseStamped global_point;

double pi = 3.141593;
ros::Publisher pose_pub;
ros::Publisher tra_pub;
nav_msgs::Path trajectory;


void vehicleMessage(const joystick2coms::VehicleMessageStamp& msg)
{
    ros_time_current = msg.header.stamp;
    steering = -(pi/180) * (msg.steering-1.32) *27/35;
    //if (abs(msg.steering)<2)
	//steering = 0;
    speed = msg.speed;
    dis = speed * ((ros_time_current - ros_time_last).toSec());
    sum_dis += dis;
    ROS_INFO("I have run %f !",sum_dis);
    ros_time_last = ros_time_current;

    kk  = asin((dis * tan(steering)/2.56));
    delta_x  = dis*sin(kk);
    delta_y  = dis*cos(kk);;
    delta_th = 2*kk;

    vb_point.header.stamp = ros::Time();
    vb_point.pose.position.x = delta_x;
    vb_point.pose.position.y = delta_y;
    vb_point.pose.position.z = 0;
    geometry_msgs::Quaternion quat = tf::createQuaternionMsgFromYaw(delta_th);
    vb_point.pose.orientation = quat;

    pose_pub.publish(global_point);
    trajectory.poses.push_back(global_point);
    tra_pub.publish(trajectory);
}

void transformPoint(const tf::TransformListener &listener)
{   	
    try
    {
	listener.transformPose("map", vb_point, global_point);
    }
    catch(tf::TransformException& ex)
    {
	ROS_ERROR(" %s", ex.what());
//	ROS_INFO("something happen");
    }	

}

int main(int argc, char** argv)
{
    vb_point.pose.orientation.w = 1;
    vb_point.header.frame_id = "coms";
    global_point.header.frame_id = "map";
    trajectory.header.frame_id = "map";

    ros::init(argc, argv, "trajectory");
    ros::NodeHandle n;
    pose_pub = n.advertise<geometry_msgs::PoseStamped>("base_pose",100);
    tra_pub = n.advertise<nav_msgs::Path>("trajectory",1);
    
    ros_time_last = ros_time_current = ros::Time();
    ros::Subscriber sub = n.subscribe("/coms/vehiclemessage",1000,vehicleMessage);

    tf::TransformListener listener(ros::Duration(0.01));  
    ros::Timer timer = n.createTimer(ros::Duration(0.01), boost::bind(&transformPoint, boost::ref(listener)));
    
    //trajectory.header.stamp = ros::Time();
    //trajectory.header.frame_id = "coms";

    ros::spin();
    return 0;
}
#include "geometry_msgs/Quaternion.h"
