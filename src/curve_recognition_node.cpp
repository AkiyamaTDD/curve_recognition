#include <ros/ros.h>
#include <std_msgs/String.h>
#include <math.h>
#include <snake_msgs/SnakeForceData.h>
#include <snake_msgs/SnakeIMUData.h>
#include <snake_msgs/SnakeCRResult.h>
#include "curve_recognition_node.h"

CurveRecognition::CurveRecognition(){
  // force_data配列初期化
  for (int i = 0; i < noj; i++) {
    for (int j = 0; j < noc; j++) {
      force_timestamp[i][j] = ros::Time::now();
      force_x[i][j] = 0;
      force_y[i][j] = 0;
      force_z[i][j] = 0;
      force_iall[i][j] = 0;
    }
  }

  for (int i = 0; i < noi; i++) {
      imu_timestamp[i] = ros::Time::now();
      imu_x[i] = 0;
      imu_y[i] = 0;
      imu_z[i] = 0;
  }

  cr_force_sub = nh.subscribe<snake_msgs::SnakeForceData>("force_data",100,&CurveRecognition::RCFCallback, this);
  cr_imu_sub = nh.subscribe<snake_msgs::SnakeIMUData>("imu_data",100,&CurveRecognition::RCICallback, this);
  cr_result_pub = nh.advertise<snake_msgs::SnakeCRResult>("curve_recognition_result", 1);
}

CurveRecognition::~CurveRecognition(){

}

void CurveRecognition::RCFCallback(const snake_msgs::SnakeForceData::ConstPtr& force_datain)
{
  int joint_index = force_datain->joint_index;
  int cop_index = force_datain->cop_index;

  ros::Time t;
  t = ros::Time::now();
  //double _dt = (t - cop_timestamp[joint_index][cop_index]).toSec();
  force_timestamp[joint_index][cop_index] = t;
  force_x[joint_index][cop_index] = force_datain->force_x;
  force_y[joint_index][cop_index] = force_datain->force_y;
  force_z[joint_index][cop_index] = force_datain->force_z;
  force_iall[joint_index][cop_index] = force_datain->iall;
}

void CurveRecognition::RCICallback(const snake_msgs::SnakeIMUData::ConstPtr& imu_datain)
{
  int imu_index = imu_datain->imu_index;

  ros::Time t;
  t = ros::Time::now();

  imu_timestamp[imu_index] = t;
  imu_x[imu_index] = imu_datain->accel_x;
  imu_y[imu_index] = imu_datain->accel_y;
  imu_z[imu_index] = imu_datain->accel_z;
}

void CurveRecognition::PublishCR(ros::Time stamp){
  snake_msgs::SnakeCRResult  cr_result;

  for (int i = 0; i < nt; i++) {
    double fx = 0.0f;
    double fy = 0.0f;
    double fz = 0.0f;
    double fiall = 0.0f;
    double ix = 0.0f;
    double iy = 0.0f;
    double iz = 0.0f;
    //重力ベクトルから見た圧力の位置ベクトルの角度方向
    double direction = 0.0f;
    //重力ベクトルから見た圧力の位置ベクトルの角度
    double theta = 0.0f;
    double cos_th = 0.0f;
    int c_dir;
    //CoPセンサとIMUの関係をリンクさせる
    int tj[nt], ti[nt];
    tj[i] = tjA;
    ti[i] = tiA;
    int tjt = tj[i];
    int tit = ti[i];

    //２枚のCoPセンサのうち力の大きい方を使用する
    for (int j = 0; j < noc; j++) {
      if (fiall<force_iall[tjt][j]) {
        fx = force_x[tjt][j];
        fy = force_y[tjt][j];
        fz = force_z[tjt][j];
        fiall = force_iall[tjt][j];
      }
    }
    //IMUのデータをCoP座標に変換
    ix = - imu_z[tit];
    iy = - imu_y[tit];
    iz = - imu_x[tit];

    ROS_INFO("Iall = %lf", fiall);
    ROS_INFO("force(y,z) = (%lf,%lf)", fy, fz);
    ROS_INFO("imu(y,z) = (%lf,%lf)", iy, iz);

    direction = (fy - 0.0)*(iz - 0.0) - (fz - 0.0)*(iy -0.0);
    cos_th = (fy*iy + fz*iz)/(sqrt(pow(fy, 2)+pow(fz, 2))+sqrt(pow(iy, 2)+pow(iz, 2)));
    theta = acos(cos_th)*180/M_PI;

    cr_result.timestamp = stamp;

    //Iallが小さい時
    if (fiall<iall_min)  {
      //重力ベクトルから見た圧力の位置ベクトルが左回りの時
      if (direction>=0) {
        if (theta>=0 && theta<22.5){
	  c_dir = 1;
	  ROS_INFO("bottom");
        }
        else if (theta>=22.5 && theta<67.5){
	  c_dir = 2;
	  ROS_INFO("bottom_right");
        }
        else if (theta>=67.5 && theta<112.5){
	  c_dir = 3;
	  ROS_INFO("right");
        }
        else if (theta>=112.5 && theta<157.5){
	  c_dir = 4;
	  ROS_INFO("upper_right");
        }
        else if (theta>=157.5 && theta<=180){
	  c_dir = 5;
	  ROS_INFO("upper");
        }
      cr_result.theta = theta;
      //重力ベクトルから見た圧力の位置ベクトルが右回りの時
      } else if (direction<0) {
        if (theta>=0 && theta<22.5){
	  c_dir = 1;
	  ROS_INFO("bottom");
        }
        else if (theta>=22.5 && theta<67.5){
	  c_dir = 8;
	  ROS_INFO("bottom_left");
        }
        else if (theta>=67.5 && theta<112.5){
	  c_dir = 7;
	  ROS_INFO("left");
        }
        else if (theta>=112.5 && theta<157.5){
	  c_dir = 6;
	  ROS_INFO("upper_left");
        }
        else if (theta>=157.5 && theta<=180){
	  c_dir = 5;
	  ROS_INFO("upper");
        }
      cr_result.theta = - theta;
      }
      cr_result.curve_direction = c_dir;
      cr_result_pub.publish(cr_result);
    }
    //Iallが大きい時
    if (fiall>iall_max) {
      //重力ベクトルから見た圧力の位置ベクトルが左回りの時
      if (direction>=0) {
        if (theta>=0 && theta<22.5){
	  c_dir = 5;
	  ROS_INFO("upper");
        }
        else if (theta>=22.5 && theta<67.5){
	  c_dir = 6;
	  ROS_INFO("upper_left");
        }
        else if (theta>=67.5 && theta<112.5){
	  c_dir = 7;
	  ROS_INFO("left");
        }
        else if (theta>=112.5 && theta<157.5){
	  c_dir = 8;
	  ROS_INFO("bottom_left");
        }
        else if (theta>=157.5 && theta<=180){
	  c_dir = 1;
	  ROS_INFO("bottom");
        }
      cr_result.theta = theta;
      //重力ベクトルから見た圧力の位置ベクトルが右回りの時
      } else if (direction<0) {
        if (theta>=0 && theta<22.5){
	  c_dir = 5;
	  ROS_INFO("upper");
        }
        else if (theta>=22.5 && theta<67.5){
	  c_dir = 4;
	  ROS_INFO("upper_right");
        }
        else if (theta>=67.5 && theta<112.5){
	  c_dir = 3;
	  ROS_INFO("right");
        }
        else if (theta>=112.5 && theta<157.5){
	  c_dir = 2;
	  ROS_INFO("botom_right");
        }
        else if (theta>=157.5 && theta<=180){
	  c_dir = 1;
	  ROS_INFO("bottom");
        }
      cr_result.theta = - theta;
      } else {
        ROS_INFO("straight");
      }
      cr_result.curve_direction = c_dir;
      cr_result_pub.publish(cr_result);
    }
  }
}



int main(int argc, char** argv) {
  ros::init(argc, argv, "curve_recognition");
  CurveRecognition cr;
  ROS_INFO("curve_recognition start");

  ros::Time time_stamp = ros::Time::now();
  ros::Rate r(50);
  while (ros::ok()){
    ros::spinOnce();
    time_stamp = ros::Time::now();

    cr.PublishCR(time_stamp);

    r.sleep();
  }

  return 0;
}
