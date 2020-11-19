#pragma once

/* includes //{ */

#include <ros/ros.h>
#include <nodelet/nodelet.h>

#include <math.h>
#include <cmath>
#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <mutex>
#include <queue>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl_conversions/pcl_conversions.h>

#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <tf_conversions/tf_eigen.h>
#include <tf2_ros/transform_broadcaster.h>

#include <eigen3/Eigen/Dense>
#include <eigen_conversions/eigen_msg.h>

#include <ceres/ceres.h>
#include <opencv/cv.h>

#include <std_srvs/SetBool.h>
#include <std_srvs/Trigger.h>

#include <std_msgs/String.h>

#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>

#include <geometry_msgs/PoseStamped.h>

#include <sensor_msgs/PointCloud2.h>

#include <mrs_lib/profiler.h>
#include <mrs_lib/param_loader.h>
#include <mrs_lib/mutex.h>
#include <mrs_lib/transformer.h>
#include <mrs_lib/attitude_converter.h>

#include "aloam_slam/common.h"
#include "aloam_slam/tic_toc.h"
#include "aloam_slam/lidarFactor.hpp"

//}

namespace aloam_slam
{
class AloamMapping {

public:
  AloamMapping(const ros::NodeHandle &parent_nh, mrs_lib::ParamLoader param_loader, std::shared_ptr<mrs_lib::Profiler> profiler, std::string frame_fcu,
               std::string frame_map, float scan_frequency, tf::Transform tf_lidar_to_fcu);

  bool is_initialized = false;

  void setData(ros::Time time_of_data, tf::Transform aloam_odometry, pcl::PointCloud<PointType>::Ptr laserCloudCornerLast,
               pcl::PointCloud<PointType>::Ptr laserCloudSurfLast, pcl::PointCloud<PointType>::Ptr laserCloudFullRes,
               aloam_slam::AloamDiagnostics::Ptr aloam_diag_msg, const float resolution_line, const float resolution_plane);

private:
  // member objects
  std::shared_ptr<mrs_lib::Profiler>             _profiler;
  std::shared_ptr<tf2_ros::TransformBroadcaster> _tf_broadcaster;

  ros::Timer _timer_mapping_loop;
  ros::Time  _time_last_map_publish;

  std::mutex                                   _mutex_cloud_features;
  std::vector<pcl::PointCloud<PointType>::Ptr> _cloud_corners;
  std::vector<pcl::PointCloud<PointType>::Ptr> _cloud_surfs;

  // Feature extractor newest data
  std::mutex                        _mutex_odometry_data;
  bool                              _has_new_data = false;
  ros::Time                         _time_aloam_odometry;
  tf::Transform                     _aloam_odometry;
  pcl::PointCloud<PointType>::Ptr   _features_corners_last;
  pcl::PointCloud<PointType>::Ptr   _features_surfs_last;
  pcl::PointCloud<PointType>::Ptr   _cloud_full_res;
  aloam_slam::AloamDiagnostics::Ptr _aloam_diag_msg;
  float                             _resolution_corners;
  float                             _resolution_surfs;

  // publishers and subscribers
  ros::Publisher _pub_laser_cloud_map;
  ros::Publisher _pub_laser_cloud_registered;
  ros::Publisher _pub_odom_global;
  ros::Publisher _pub_path;
  ros::Publisher _pub_diag;

  // services
  ros::ServiceServer _srv_reset_mapping;

  // ROS messages
  nav_msgs::Path::Ptr _laser_path_msg      = boost::make_shared<nav_msgs::Path>();
  Eigen::Vector3d     _path_last_added_pos = Eigen::Vector3d::Identity();

  // member variables
  std::string _frame_fcu;
  std::string _frame_map;

  float _scan_frequency;
  float _mapping_frequency;
  float _map_publish_period;

  tf::Transform _tf_lidar_to_fcu;

  double                         _parameters[7] = {0, 0, 0, 1, 0, 0, 0};
  Eigen::Map<Eigen::Quaterniond> _q_w_curr;
  Eigen::Map<Eigen::Vector3d>    _t_w_curr;

  // wmap_T_odom * odom_T_curr = wmap_T_curr;
  // transformation between odom's world and map's world frame
  Eigen::Quaterniond _q_wmap_wodom;
  Eigen::Vector3d    _t_wmap_wodom;

  Eigen::Quaterniond _q_wodom_curr;
  Eigen::Vector3d    _t_wodom_curr;

  long int _frame_count           = 0;
  float    _total_running_time_ms = 0.0f;

  int       _cloud_center_width  = 10;
  int       _cloud_center_height = 10;
  int       _cloud_center_depth  = 5;
  const int _cloud_width         = 21;
  const int _cloud_height        = 21;
  const int _cloud_depth         = 11;
  const int _cloud_volume        = _cloud_width * _cloud_height * _cloud_depth;  // 4851

  // member methods
  void timerMapping([[maybe_unused]] const ros::TimerEvent &event);
  bool callbackResetMapping(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res);

  void transformAssociateToMap();
  void transformUpdate();
  void pointAssociateToMap(PointType const *const pi, PointType *const po);
};
}  // namespace aloam_slam
