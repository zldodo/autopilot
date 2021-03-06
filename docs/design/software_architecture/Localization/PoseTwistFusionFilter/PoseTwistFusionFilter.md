# Pose Twist Fusion Filter

## Role

Pose Twist Fusion Filter is a component to integrate the poses estimated by pose estimator and the twists estimated by twist estimator considering the time delay of sensor data. This component improves the robustness of localization, e.g. even when NDT scan matching is unreliable, a vehicle can keep autonomous driving based on vehicle twist information.

## Input

| Input           | Data Type                                   |
| --------------- | ------------------------------------------- |
| Initial Pose    | `geometry_msgs::PoseWithCovarianceStamped`  |
| Estimated Pose  | `geometry_msgs::PoseWithCovarianceStamped`  |
| Estimated Twist | `geometry_msgs::TwistWithCovarianceStamped` |

## Output

| Output                       | Data Type                                  | Use Cases of the output       |
| ---------------------------- | ------------------------------------------ | ----------------------------- |
| Vehicle Pose                 | `tf2_msgs::TFMessage`                      | Perception, Planning, Control |
| Vehicle Pose with Covariance | `geometry_msgs::PoseWithCovarianceStamped` | Control                       |
| Vehicle Twist                | `geometry_msgs::TwistStamped`              | Planning, Control             |

## Design

![Pose_Twist_Fusion_Filter](image/PoseTwistFusionFilter.drawio.svg)

Estimated pose in Pose Estimator and estimated twist in Twist Estimator basically contain error to some extent.
In order to integrate the 2D vehicle dynamics model with the estimated pose and twist of ego vehicle and generate a robust and less noisy pose and twist, we implement an extended kalman filter(EKF) as Pose Twist Fusion Filter shown in the figure above.
If you would like to know further details of EKF model, please see [this document](/src/localization/pose_twist_fusion_filter/ekf_localizer/README.md).

The stop filter detects the vehicle's stop state by monitoring both velocity and angular velocity.
When so, the stop filter filters the velocity and angular velocity to 0.
