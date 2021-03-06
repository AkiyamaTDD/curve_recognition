#include <ros/ros.h>
#include <std_msgs/String.h>
#include <snake_msgs/SnakeCOPData.h>
#include <snake_msgs/SnakeIMUData.h>
#include <snake_msgs/SnakeCRResult.h>

#define nol (21)    // リンク数
#define noj (nol-1) // 関節数
#define noc (2)       // 接触センサ数
#define noi (1)	    // IMUの数

#define nt (1)      //使用するフィードバック情報の数
#define tjA (1)      //1番目のフィードバックに使用するCoPセンサのジョイント
#define tiA (0)      //1番目のフィードバックに使用するimuセンサの番号

#define iall_min (0.001)
#define iall_max (0.003)


class CurveRecognition {
public:
  CurveRecognition();
  ~CurveRecognition();
  void PublishCR(ros::Time);

private:

  //圧力データ
  double force_x[noj][noc];
  double force_y[noj][noc];
  double force_z[noj][noc];
  double force_iall[noj][noc];
  ros::Time force_timestamp[noj][noc];

  //IMUデータ
  double imu_x[noi];
  double imu_y[noi];
  double imu_z[noi];
  ros::Time imu_timestamp[noi];

  void RCFCallback(const snake_msgs::SnakeForceData::ConstPtr&);
  void RCICallback(const snake_msgs::SnakeIMUData::ConstPtr&);
  ros::NodeHandle nh;
  ros::Subscriber cr_force_sub;
  ros::Subscriber cr_imu_sub;
  ros::Publisher cr_result_pub;
};
