# Porting ROS1 code to ROS2

## Setting up the environment

Do the following to setup and start ADE environment

1. Setup ade home

   ```sh
   cd ~
   mkdir ade-home
   cd ade-home
   touch .adehome
   ```

2. Setup AutowareArchitectureProposal

   ```sh
   git clone https://github.com/tier4/AutowareArchitectureProposal
   cd AutowareArchitectureProposal
   git checkout ros2
   ```

3. enter ADE

   ```sh
   cd ~/ade-home/AutowareArchitectureProposal
   ade start --update --enter
   cd AutowareArchitectureProposal
   ```

   All commands that follow are to be entered in ADE. Next step is to fetch the sub-repos:

   ```sh
   cd ~/AutowareArchitectureProposal
   mkdir src
   vcs import src < autoware.proj.repos
   rosdep update
   rosdep install -y --from-paths src --ignore-src --rosdistro foxy
   colcon build --event-handlers console_cohesion+
   ```

   For instance, the `shift_decider` package is in the repository `github.com:tier4/pilot.auto.git`, which is now in the `autoware/pilot.auto` subdirectory.

   Now branch off `ros2` inside that subdirectory and delete the `COLCON_IGNORE` file in the package you want to port.

## Important changes

The best source on migrating is the [migration guide](https://index.ros.org/doc/ros2/Contributing/Migration-Guide/). It doesn't mention everything though, so this section lists some areas with important changes.

A good general strategy is to try to implement those changes, then iteratively run `colcon build --packages-up-to <your_package>` and fix the first compiler error.

### Rewriting `package.xml`

The migration guide covers this well. See also [here](https://www.ros.org/reps/rep-0149.html) for a reference of the most recent version of this format.

#### When to use which dependency tag

Any build tool needed only to set up the build needs `buildtool_depend`; e.g.,

```xml
<buildtool_depend>ament_cmake</buildtool_depend>
<buildtool_depend>rosidl_default_generators</buildtool_depend>
```

Any external package included with `#include` in the files (source or headers) needs to have a corresponding `<build_depend>`; e.g.,

```xml
<build_depend>logging</build_depend>
```

Any external package included with `#include` in the header files also needs to have a corresponding `<build_export_depend>`; e.g.,

```xml
<build_export_depend>eigen</build_export_depend>
```

Any shared library that needs to be linked when the code is executed needs to have a corresponding `<exec_depend>`: this describes the runtime dependencies; e.g.,

```xml
<exec_depend>std_msgs</exec_depend>
<exec_depend>rosidl_default_runtime</exec_depend>
```

If a package falls under all three categories (`<build_depend>`, `<build_export_depend>`, and `<exec_depend>`), it is possible to just use `<depend>`

```xml
<depend>shift_decider</depend>
```

### Rewriting `CMakeLists.txt`

This is not always straightforward. A starting point is to look at the [pub-sub tutorial](https://index.ros.org/doc/ros2/Tutorials/Writing-A-Simple-Cpp-Publisher-And-Subscriber/#cpppubsub). This uses `ament_cmake`, which has a [relatively good guide](https://index.ros.org/doc/ros2/Tutorials/Ament-CMake-Documentation/) that still doesn't cover everything (like what to do when installing an executable).

#### `ament_cmake` and `ament_cmake_auto`

One drawback of `ament_cmake` is that it requires typing out the dependencies at least twice, once in `package.xml` and once or more in `CMakeLists.txt`.

Another possibility is to use `ament_auto` to get terse `CMakeLists.txt`. See [this commit](https://github.com/tier4/Pilot.Auto/pull/7/commits/ef382a9b430fd69cb0a0f7ca57016d66ed7ef29d) for an example. Unfortunately, there is no documentation for this tool, so you can only learn it from examples and reading the source code. It is also limited in what it does – it cannot currently generate message definitions, for instance, and always links all dependencies to all targets.

There are more subtle issues too, like `ament_auto_find_build_dependencies()`. It just takes all the build dependencies verbatim from `package.xml` and calls `find_package()` without the `REQUIRED` option. This causes an issue when your library has a different name in `package.xml`/`rosdep` and in `CMake`. It also removes safety, as this won't complain when you mistype a package name or haven't installed the package yet. For these reasons, it's recommended to do the following:

```cmake
# Mark all packages as REQUIRED
ament_auto_find_build_dependencies(REQUIRED
  ${${PROJECT_NAME}_BUILD_DEPENDS}
  ${${PROJECT_NAME}_BUILDTOOL_DEPENDS}
)
```

#### C++ standard

Add the following to ensure that a specific standard is required and extensions are not allowed

```cmake
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()
```

#### Compiler flags

Make sure that flags are added only for specific compilers. Not everyone uses `gcc` or `clang`.

```cmake
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()
```

#### Linters

Add only `ament_cmake_cppcheck` to the list of linters in `package.xml`

```xml
  <test_depend>ament_lint_auto</test_depend>
  <test_depend>ament_cmake_cppcheck</test_depend>
```

And the corresponding code in `CMakeLists.txt`

```cmake
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()
```

Additionally, we use `clang-tidy`, in order for it to work, we need to build packages with the following:

```shell
colcon build --packages-up-to <pkg_name> --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

And then:

```shell
clang-tidy -p build/compile_commands.json <path_to_pkg_source>
```

To check the output of the linters we can just run the tests with:

```shell
colcon test --packages-select <pkg_name> && colcon test-result --verbose
```

### Replacing `std_msgs`

In ROS2, you should define semantically meaningful wrappers around primitive (number) types. They are deprecated in Foxy.

### Changing the namespaces and header files for generated message types

If you follow the migration guide and change the included headers to have an extra `/msg` in the path and convert to `snake_case`, you might get a cryptic error. Turns out _two_ files are being generated: One for C types (`.h` headers) and one for CPP types (`.hpp` headers). So don't forget to change `.h` to `.hpp` too. Also, don't forget to insert an additional `::msg` between the package namespace and the class name.

A tip: Sublime Text has a handy "Case Conversion" package for converting to snake case.

### Adapting message definitions

If your message included something like `Header header`, it needs to be changed to `std_msgs/Header header`. Otherwise you'll see messages like `fatal error: autoware_api_msgs/msg/detail/header__struct.hpp: No such file or directory`.

### Inheriting from Node instead of NodeHandle members

That's where differences start to show – I decided to make the `VehicleCmdGate` a `Node` even though the filename would suggest that `vehicle_cmd_gate_node.cpp` would be it. That's because it has publishers, subscribers, and logging. It previously had _two_ NodeHandles, a public one and a private one (`"~"`). The public one was unused and could be removed. Private nodes are not supported in ROS2, so I simply made it public, but that is an area that needs to be brought up in review.

### Latched topics

For each latched publisher, you can use `transient_local` durability QoS on the publisher, e.g. when the history depth is 1:

```cpp
rclcpp::QoS durable_qos{1};
durable_qos.transient_local();
```

or

```cpp
rclcpp::QoS durable_qos = rclcpp::QoS(1).transient_local();
```

However, all subscribers to that topic will also need `transient_local` durability. If this is omitted, the connection between the two will be negotiated to be volatile, i.e. old messages will not be delivered when the subscriber comes online.

### Timing issues

First, if the timer can be replaced with a data-driven pattern, it is the preferred alternative for the long term:

#### The Problem with Timer-Driven Patterns

It is well understood that a polling or timer-driven pattern increases jitter (i.e. variance of latency). (Consider, for example: if every data processing node in a chain operates on a timer what is the best and worst case latency?) As a consequence for more timing-sensitive applications, it is generally not preferred to use a timer-driven pattern.

On top of this, it is also reasonably well known that [use of the clock is nondeterministic](https://martinfowler.com/articles/nonDeterminism.html) and internally this has been a large source of frustration with bad, or timing sensitive tests. Such tests typically require specific timing and/or implicitly require a certain execution order (loosely enforced by timing assumptions rather than explicitly via the code).

As a whole, introducing the clock explicitly (or implicitly via timers) is problematic because it introduces additional state, and thus assumptions on the requirements for the operation of the component. Consider also leap seconds and how that might ruin the operation and/or assumptions needed for the proper operation of the component.

#### Preferred Patterns

In general, a data-driven pattern should be preferred to a timer-driven pattern. One reasonable exception to this guideline is the state estimator/filter at the end of localization. A timer-driven pattern in this context is useful to provide smooth behavior and promote looser coupling between the planning stack and the remainder of the stack.

The core idea behind a data-driven pattern is that as soon as data arrives, it should be appropriately processed. Furthermore, the system clock (or any other source of time) should not be used to manipulate data or the timestamps. This pattern is valuable since it implicitly cuts down on hidden state (being the clock), and thus simplifies assumptions needed for the node to work.

For examples of this kind of pattern, see the lidar object detection stack in Autoware.Auto. By not using any mention of the clock save for in the drivers, the stack can run equivalently on bag data, simulation data, or live data. A similar pattern with multiple inputs can be seen in the MPC implementation both internally and externally.

#### Replicating `ros::Timer`

Assuming you still want to replicate the existing `ros::Timer` functionality: There is `rclcpp::WallTimer`, which has a similar interface, but it's not equivalent. The wall timer uses a wall clock (`RCL_STEADY_TIME` clock), i.e. it doesn't listen to the `/clock` topic populated by simulation time. That the timer doesn't stop when simulation time stops, and doesn't go faster/slower when simulation time goes faster or slower.

By contrast, the `GenericTimer` provides an interface to supply a clock, but there is no convenient function for setting up such a timer, comparable to `Node::create_wall_timer`. For now, this works:

```cpp
auto timer_callback = std::bind(&VehicleCmdGate::onTimer, this);
auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
  std::chrono::duration<double>(update_period_));
timer_ = std::make_shared<rclcpp::GenericTimer<decltype(timer_callback)>>(
  this->get_clock(), period, std::move(timer_callback),
  this->get_node_base_interface()->get_context());
this->get_node_timers_interface()->add_timer(timer_, nullptr);
```

Also, this doesn't work, even with a subsequent `add_timer()` call:

```cpp
timer_ = rclcpp::create_timer(this, this->get_clock(), period, timer_callback);
```

#### Rosbag recording

Unfortunately, one additional problem remains. `ros2 bag` does not record `/clock` (aka sim time) whereas `rosbag` does. This implies that in order to get the same behavior in ROS 2, either:

- `rosbag` along with the `ros1_bridge` must be used
- Some explicit time source must be used and explicitly recorded by `ros2 bag`

### Parameters

It's not strictly necessary, but you probably want to make sure the filename is `xyz.param.yaml`. Then come two steps:

#### Adjust code

```cpp
double vel_lim;
pnh_.param<double>("vel_lim", vel_lim, 25.0);
```

becomes

```cpp
const double vel_lim = declare_parameter("vel_lim", 25.0);
```

which is equivalent to

```cpp
const double vel_lim = declare_parameter<double>("vel_lim", 25.0);
```

This allows to set the initial value e.g. via a parameter file.

**NOTE** Calling `ros2 param set <NODE> vel_lim 1.234` after starting the node works but will not
alter the member `vel_lim`! See the section below on _dynamic reconfigure_ to achieve that.

### `dynamic_reconfigure`

Dynamic reconfigure as it existed in ROS1 does not exist anymore in ROS2 and can be achieved by
simpler means using a parameter callback.

#### `cfg` files

Remove the package's `.cfg` file and associated `cfg/` subdirectory.

#### Header file

In the header file, remove includes of `dynamic_reconfigure` and the node-specific config file. As a concrete example,
take the [MPC follower](https://github.com/tier4/Pilot.Auto/pull/52)

```diff
-#include <dynamic_reconfigure/server.h>
-#include <mpc_follower/MPCFollowerConfig.h>
```

you need to set a parameter handler and callback function:

```cpp
OnSetParametersCallbackHandle::SharedPtr set_param_res_;
rcl_interfaces::msg::SetParametersResult paramCallback(const std::vector<rclcpp::Parameter> & parameters);
```

If there are many parameters (rule of thumb: more than 2), it is more practical to group them in a
struct defined within the node's declaration:

```cpp
class MPCFollower : public rclcpp::Node
{

  struct MPCParam
  {
    int prediction_horizon;
    ...
  } mpc_param;

};

```

Add a method to declare all the parameters

```cpp
void declareMPCparameters();
```

#### Implementation file

Write the following into the definition of the class that inherits from `rclcpp::Node`.

A few macros and a utility function can help keep the following code clean and void of redundancy. These macros are optional but help when many parameters are to be updated dynamically.

```cpp
#define DECLARE_MPC_PARAM(PARAM_STRUCT, NAME, VALUE) \
  PARAM_STRUCT.NAME = declare_parameter("mpc_" #NAME, VALUE)

#define UPDATE_MPC_PARAM(PARAM_STRUCT, NAME) \
  update_param(parameters, "mpc_" #NAME, PARAM_STRUCT.NAME)

namespace
{
template <typename T>
void update_param(
  const std::vector<rclcpp::Parameter> & parameters, const std::string & name, T & value)
{
  auto it = std::find_if(parameters.cbegin(), parameters.cend(),
    [&name](const rclcpp::Parameter & parameter) { return parameter.get_name() == name; });
  if (it != parameters.cend()) {
    value = it->template get_value<T>();
  }
}
}  // namespace
```

In the constructor, define the callback and declare parameters with default values

```cpp
// the type of the ROS parameter is defined by the C++ type of the default value,
// so 50 is not equivalent to 50.0!
DECLARE_MPC_PARAM(mpc_param_, prediction_horizon, 50);

// set parameter callback
set_param_res_ = add_on_set_parameters_callback(std::bind(&MPCFollower::paramCallback, this, _1));
```

Inside the callback, you have to manually update each parameter for which you want to react to change from the outside. You can (inadvertently) declare more parameters than you react to.

```cpp
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  result.reason = "success";

  // strong exception safety wrt MPCParam
  MPCParam param = mpc_param_;
  try {
    UPDATE_MPC_PARAM(param, prediction_horizon);
    // update all other parameters, too

    // transaction succeeds, now assign values
    mpc_param_ = param;
  } catch (const rclcpp::exceptions::InvalidParameterTypeException & e) {
    result.successful = false;
    result.reason = e.what();
  }

  return result;
```

When the node is running, you can set the parameter dynamically with the following command. The value is converted to a type as in C++, so setting an `int` to 51.0 leads to an `InvalidParameterTypeException` in the callback.

```sh
ros2 param set /mpc_follower prediction_horizon 51
```

Make sure relevant parameters can be set from the command line or `rqt` and changes are reflected by
the package.

#### Parameter client [discouraged]

The parameter client is another way to dynamically set the parameter defined in the node. The client subscribes to the `/parameter_event` topic and call the callback function. This allows the client node to get all the information about parameter changes in every node. The callback argument contains the target node name, which can be used to determine which node the parameter change is for.

In .hpp,

```cpp
rclcpp::AsyncParametersClient::SharedPtr param_client_;
rclcpp::Subscription<rcl_interfaces::msg::ParameterEvent>::SharedPtr sub_param_event_;

void paramCallback(const rcl_interfaces::msg::ParameterEvent::SharedPtr event);
```

In .cpp,

```cpp
// client setting
param_client_ = std::make_shared<rclcpp::AsyncParametersClient>(this, "param_client");
sub_param_event_ =
  param_client_->on_parameter_event(std::bind(&VelocityController::paramCallback, this, std::placeholders::_1));

// callback setting
void paramCallback(const rcl_interfaces::msg::ParameterEvent::SharedPtr event)
{
  for (auto & new_parameter : event->new_parameters) {
      std::cout << "  " << new_parameter.name << std::endl;
  }
  for (auto & changed_parameter : event->changed_parameters) {
      std::cout << "  " << changed_parameter.name << std::endl;
  }
  for (auto & deleted_parameter : event->deleted_parameters) {
      std::cout << "  " << deleted_parameter.name << std::endl;
  }
};
```

However, this method calls the callback for all parameter changes of all nodes. So the `add_on_set_parameters_callback` is recommended for the porting of the dynamic reconfigure.

reference:

- <https://discourse.ros.org/t/composition-and-parameters-best-practice-suggestions/1001>
- <https://github.com/ros2/rclcpp/issues/243>

#### Adjust param file

Two levels of hierarchy need to be added around the parameters themselves and each level has to be indented relative to its parent (by two spaces in this example):

```xml
<node name or /**>:
  ros__parameters:
    <params>
```

##### Types

Also, ROS1 didn't have a problem when you specify an integer, e.g. `28` for a `double` parameter, but ROS2 does:

```sh
[vehicle_cmd_gate-1] terminate called after throwing an instance of 'rclcpp::exceptions::InvalidParameterTypeException'
[vehicle_cmd_gate-1]   what():  parameter 'vel_lim' has invalid type: expected [double] got [integer]
```

Best to just change `28` to `28.0` in the param file. See also [this issue](https://github.com/ros2/rclcpp/issues/979).

### Launch file

There is a [migration guide](https://index.ros.org/doc/ros2/Tutorials/Launch-files-migration-guide/). One thing it doesn't mention is that the `.launch` file also needs to be renamed to `.launch.xml`.

### Replacing `tf2_ros::Buffer`

A `tf2_ros::Buffer` member that is filled by a `tf2_ros::TransformListener` can become a `tf2::BufferCore` in most cases. This reduces porting effort, since the a `tf2::BufferCore` can be constructed like a ROS1 `tf2_ros::Buffer`. For an example, see [this PR](https://github.com/tier4/Pilot.Auto/pull/11).

However, in some cases the extra functionality of `tf2_ros::Buffer` is needed. For instance, waiting for a transform to arrive, usually in the form of `lookupTransform()` with a timeout argument.

#### Avoiding a lookup with timeout

Often, code doesn't really need a transform lookup with timeout. For instance, [this package](https://github.com/tier4/Pilot.Auto/pull/80/files#diff-1fd60e4ec61c376d6b6b088a7878676408a5ac6a665977590610be92d5079e55L270-L277) has a "main" subscription callback, `onTrigger()`, that waits for the most recent transform (`tf2::TimePointZero`), then checks if auxiliary data from other subscriptions is there and returns early from the callback if it isn't. In that case, I think the callback can simply treat transforms the same way as this auxiliary data, i.e. just do a simple `lookupTransform()` with no timeout and return early from the callback if it fails. The node won't do any work anyway until it's ready (i.e. has all the auxiliary data). Note that this pattern works only when the node is waiting for the most recent transform – if your callback wants to use the transform at a specific time, e.g. the timestamp of the message that triggered the callback, this pattern doesn't make sense. In that case, avoiding `waitForTransform()` requires refactoring the architecture of your system, but that topic is currently out of scope.

It's worth keeping in mind that waiting for transforms in general can be troublesome – it is only a probabilistic solution that fails in a bad way for latency spikes, makes it hard to reason about the behavior of the whole system and probably incurs more latency than necessary.

#### When you can't avoid a lookup with timeout

You should be able to use `tf2_ros::Buffer::lookupTransform()` with a timeout out of the box. There is one caveat, namely there has been at least one report of such an error:

```txt
Do not call canTransform or lookupTransform with a timeout unless you are using another thread for populating data.
Without a dedicated thread it will always timeout.
If you have a separate thread servicing tf messages, call setUsingDedicatedThread(true) on your Buffer instance.
```

There is also the drawback of spamming the console with warnings like

```sh
Warning: Invalid frame ID "a" passed to canTransform argument target_frame - frame does not exist
          at line 133 in /tmp/binarydeb/ros-foxy-tf2-0.13.6/src/buffer_core.cpp
```

if the frame has never been sent before.

#### What about `waitForTransform()`?

There is also a [`waitForTransform` API](http://docs.ros2.org/foxy/api/tf2_ros/classtf2__ros_1_1Buffer.html#a832b188dd65cbec52cabc1ec07856e49) involving futures, but it has bugs and limitations. In particular:

- Its callback does not get called when the request can be answered immediately
- You can not call `get()` on the future and expect it to return the transform or throw an exception when the timeout ends. It continues waiting after the timeout expires if the future is not ready yet (i.e. the transform hasn't arrived).

This limits you to the following style of calling the API, which doesn't use the callback and limits the waiting time before calling `get()` on the future:

```cpp
// In the node definition
tf2_ros::Buffer tf_buffer_;
tf2_ros::TransformListener tf_listener_;
...
// In the constructor
auto cti = std::make_shared<tf2_ros::CreateTimerROS>(this->get_node_base_interface(), this->get_node_timers_interface());
tf_buffer_.setCreateTimerInterface(cti);
...
// In the function processing data
auto tf_future = tf_buffer_.waitForTransform(a, b, msg_time, std::chrono::milliseconds(0), [this](auto){});
auto status = tf_future.wait_for(timeout_);
if (status == std::future_status::deferred) {
  // This never happened in experiments
} else if (status == std::future_status::timeout) {
  // The transform did not arrive within the timeout duration
} else {
  // The transform is here, and can now be accessed without triggering the waiting-infinitely bug
  auto transform = tf_future.get();
}
```

The `waitForTransform()` function will return immediately. Note that the timeout passed to `waitForTransform()` does not matter, only the timeout passed to `wait_for()`.

There is a bug with this when `tf2::TimeStampZero` is requested instead of a nonzero time: the status will be `ready`, but accessing the result with `get()` throws an exception. There is another bug where this bug is not triggered under some conditions, for extra fun. So do not use `waitForTransform()` with `tf2::TimeStampZero` (or any other way of saying "time 0"). The silver lining is that this can be often avoided anyway since it is the scenario described in the section before.

For more details, see <https://github.com/nnmm/tf2_example> and the [resulting table](https://docs.google.com/spreadsheets/d/1BvFIMwp0kSkQecw2dkkyj4kR9edFLRDDBBTGynHBAN8/edit?usp=sharing).

### Shared pointers

Be careful in creating a `std::shared_ptr` to avoid a double-free situation when the constructor argument after porting is a dereferenced shared pointer. For example, if `msg` previously was a raw pointer and now is a shared pointer, the following would lead to both `msg` and `a` deleting the same resource.

```cpp
auto a = std::make_shared<autoware_planning_msgs::Trajectory>(*msg);
```

To avoid this, just copy the shared pointer

```cpp
std::shared_ptr<autoware_planning_msgs::msg::Trajectory> a = msg;
```

### Service clients

There is no synchronous API for service calls, and the futures API can not be used from inside a node, only the callback API.

The futures API is what is used in tutorials such as [Writing a simple service and client](https://index.ros.org/doc/ros2/Tutorials/Writing-A-Simple-Cpp-Service-And-Client/#write-the-client-node), but note that the call to `rclcpp::spin_until_future_complete()` does not happen from inside any subscriber callback or similar. If you do call it from inside a node, you will get

```sh
    terminate called after throwing an instance of 'std::runtime_error'
      what():  Node has already been added to an executor.
```

The node itself is already added to the executor in `rclcpp::spin()` function inside the main function, and `rclcpp::spin_until_future_complete()` tries to add the node to another executor.

You might note that the function already returned a `std::shared_future` on which you could wait. But that just hangs forever.

So you're left with using a [callback](http://docs.ros2.org/foxy/api/rclcpp/classrclcpp_1_1Client.html#a62e48edd618bcb73538bfdc3ee3d5e63). Unfortunately that leaves you with no option to handle failure: For instance, if the service dies, your callback will never get called. There's no easy way to say that you only want to wait for 2 seconds for a result.

Another idea for a workaround is to do something similar to what is done in the `rclcpp::spin_until_future_complete()` function by ourselves. Another possible avenue is using multithreaded executors, see [this post](https://answers.ros.org/question/343279/ros2-how-to-implement-a-sync-service-client-in-a-node/) for some more detail.

### Logging

The node name is now automatically prepended to the log message, so that part can be removed. In methods, get the logger from the node with `get_logger()`. In a free function `foo()`, use `rclcpp::get_logger("foo")`. To provide a further level of hierarchy, use `get_logger("foo").get_child("bar")`.

For example,

```cpp
ROS_INFO_COND(show_debug_info_, "[MPC] some message with a float value %g", some_member_);
```

should become

```cpp
RCLCPP_INFO_EXPRESSION(get_logger(), show_debug_info_, "some message with a float value %g", some_member_);
```

The mapping of logger macros is basically just

```diff
-ROS_INFO(...)
+RCLCPP_INFO(get_logger(), ...)
```

with the exception of

```diff
-ROS_INFO_COND(cond, ...)
+RCLCPP_INFO_EXPRESSION(logger, cond, ...)

-ROS_WARN_DELAYED_THROTTLE(duration, ...)
+RCLCPP_WARN_SKIPFIRST_THROTTLE(get_logger(), *get_clock(), duration, ...)
```

where the `duration` is an integer interpreted as milliseconds as opposed to seconds in ROS1. A readable way to document that is

```cpp
RCLCPP_WARN_SKIPFIRST_THROTTLE(get_logger(), *get_clock(), 5000 /* ms */, ...)
```

### Shutting down a subscriber

The `shutdown()` method doesn't exist anymore, but you can just throw away the subscriber with `this->subscription_ = nullptr;` or similar, for instance inside the subscription callback. Curiously, this works even though the `subscription_` member variable is not the sole owner – the `use_count` is 3 in the `minimal_subscriber` example.

### Durations

Beware of just replacing `ros::Duration` with `rclcpp::Duration` – it compiles, but now expects nanoseconds instead of seconds. Use `rclcpp::Duration::from_seconds` instead.

## Alternative: Semi-automated porting with ros2-migration-tools (not working)

**The following instructions to use `ros2-migration-tools` are given for completeness, we gave up and decided to port packages manually.**

From <https://github.com/awslabs/ros2-migration-tools>:

```sh
pip3 install parse_cmake
git clone https://github.com/awslabs/ros2-migration-tools.git
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
tar xaf clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz
cp -r clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/libclang.so ros2-migration-tools/clang/
cp -r clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/libclang.so.10 ros2-migration-tools/clang/
cp -r clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/include/ ros2-migration-tools/clang/clang
```

The package needs to be **built with ROS1**. I followed <http://wiki.ros.org/noetic/Installation/Ubuntu> outside of ADE

```sh
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
sudo apt update
sudo apt install ros-melodic-ros-base
```

In <https://github.com/awslabs/ros2-migration-tools#setup-the-ros1-packages>, it instructs me to compile a ros1 package with colcon to get started.

```sh
colcon build --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

And that fails. I thought `colcon` is only for ROS2 so it should only work after porting, not before. I retried with `catkin_make` but also ran into issues there

```sh
frederik.beaujean@frederik-beaujean-01:~/ade-home/AutowareArchitectureProposal$ catkin_make --source src/autoware/autoware.iv/control/shift_decider/ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
Base path: /home/frederik.beaujean/ade-home/AutowareArchitectureProposal
Source space: /home/frederik.beaujean/ade-home/AutowareArchitectureProposal/src/autoware/autoware.iv/control/shift_decider
Build space: /home/frederik.beaujean/ade-home/AutowareArchitectureProposal/build
Devel space: /home/frederik.beaujean/ade-home/AutowareArchitectureProposal/devel
Install space: /home/frederik.beaujean/ade-home/AutowareArchitectureProposal/install
####
#### Running command: "cmake /home/frederik.beaujean/ade-home/AutowareArchitectureProposal/src/autoware/autoware.iv/control/shift_decider -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCATKIN_DEVEL_PREFIX=/home/frederik.beaujean/ade-home/AutowareArchitectureProposal/devel -DCMAKE_INSTALL_PREFIX=/home/frederik.beaujean/ade-home/AutowareArchitectureProposal/install -G Unix Makefiles" in "/home/frederik.beaujean/ade-home/AutowareArchitectureProposal/build"
####
CMake Error: The source "/home/frederik.beaujean/ade-home/AutowareArchitectureProposal/src/autoware/autoware.iv/control/shift_decider/CMakeLists.txt" does not match the source "/home/frederik.beaujean/ade-home/AutowareArchitectureProposal/src/CMakeLists.txt" used to generate cache.  Re-run cmake with a different source directory.
Invoking "cmake" failed
```

According to an expert:

> You should be able to compile it with colcon, cause it works for both ROS 1 and ROS 2 code. You are getting the same error with catkin so it's probably something related to ROS 1 and the build instructions.

## Strange errors and their causes

Some error messages are so unhelpful that it might help to collect them and their causes.

### package.xml errors

If you forget `<build_type>ament_cmake</build_type>`, or you use package format 2 in combination with a package format 3 tag like `<member_of_group>`, you'll get the unhelpful error

```sh
CMake Error at /usr/share/cmake-3.16/Modules/FindPackageHandleStandardArgs.cmake:146 (message):
  Could NOT find FastRTPS (missing: FastRTPS_INCLUDE_DIR FastRTPS_LIBRARIES)
```

### YAML param file

#### Tabs instead of spaces

Used tabs instead of spaces in your param.yaml file? _Clearly_, the most user-friendly error message is

```sh
$ ros2 launch mypackage mypackage.launch.xml
[INFO] [launch]: All log files can be found below /home/user/.ros/log/2020-10-19-19-09-13-676799-t4-30425
[INFO] [launch]: Default logging verbosity is set to INFO
[INFO] [mypackage-1]: process started with pid [30427]
[mypackage-1] [ERROR] [1603127353.755503075] [rcl]: Failed to parse global arguments
[mypackage-1]
[mypackage-1] >>> [rcutils|error_handling.c:108] rcutils_set_error_state()
[mypackage-1] This error state is being overwritten:
[mypackage-1]
[mypackage-1]   'Couldn't parse params file: '--params-file /home/user/workspace/install/mypackage/share/mypackage/param/myparameters.yaml'. Error: Error parsing a event near line 1, at /tmp/binarydeb/ros-foxy-rcl-yaml-param-parser-1.1.8/src/parse.c:599, at /tmp/binarydeb/ros-foxy-rcl-1.1.8/src/rcl/arguments.c:391'
[mypackage-1]
[mypackage-1] with this new error message:
[mypackage-1]
[mypackage-1]   'context is zero-initialized, at /tmp/binarydeb/ros-foxy-rcl-1.1.8/src/rcl/context.c:51'
[mypackage-1]
[mypackage-1] rcutils_reset_error() should be called after error handling to avoid this.
[mypackage-1] <<<
[mypackage-1] [ERROR] [1603127353.755523149] [rclcpp]: failed to finalize context: context is zero-initialized, at /tmp/binarydeb/ros-foxy-rcl-1.1.8/src/rcl/context.c:51
[mypackage-1] terminate called after throwing an instance of 'rclcpp::exceptions::RCLInvalidROSArgsError'
[mypackage-1]   what():  failed to initialize rcl: error not set
[ERROR] [mypackage-1]: process has died [pid 30427, exit code -6, cmd '/home/user/workspace/install/mypackage/lib/mypackage/mypackage --ros-args -r __node:=mypackage --params-file /home/user/workspace/install/mypackage/share/mypackage/param/myparameters.yaml'].
```

and that is indeed what ROS2 will tell you.

#### No indentation

Without proper indentation of levels, there is a segfault when the YAML is parsed during `rclcpp::init(argc, argv)`. The error is similar to the above but begins with

```sh
[mpc_follower-1] free(): double free detected in tcache 2
```

Note that this message may be hidden when just launching with `ros2 launch`. It is shown running the node under `valgrind` which requires a `launch-prefix`. For example, modify `mpc_follower.launch.xml`

```xml
<node pkg="mpc_follower" exec="mpc_follower" name="mpc_follower" output="screen" launch-prefix="valgrind">
```

Another helpful option for diagnosing segfaults is to run under `gdb` to get a backtrace. Change the prefix

```xml
<node pkg="mpc_follower" exec="mpc_follower" name="mpc_follower" output="screen" launch-prefix="xterm -e gdb -ex run --args">
```

and after the segfault occurred, you can enter `bt` in the `xterm` window.
