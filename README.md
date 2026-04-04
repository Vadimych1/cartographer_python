# Installing CartoPy

For installing package you first need to build a wheel. Required packages are:
- google-mock
- ceres-solver
- lua5.3
- protobuf
- protobuf-compiler
- boost
- gflags
- glog
- atlas
- eigen3
- cmake
- ninja-build
- unwind
- absl
- cairo

If you are using Ubuntu/Debian:
```bash
sudo apt-get install -y \
    google-mock libceres-dev liblua5.3-dev \
    libprotobuf-dev protobuf-compiler \
    libboost-all-dev libgflags-dev \
    libgoogle-glog-dev libatlas-base-dev \
    libeigen3-dev cmake ninja-build \
    libunwind-dev libabsl-dev libcairo-dev
```

Next run this in root directory. It will output .whl file into ./dist/ that you can install (requires python3 and python3-build module):
```bash
python3 -m build
```

# Example using:
```python
import carto_py as cp

# creating cartographer wrapper
w = cp.Carto("lidar", "imu") # passing lidar and IMU sensor names for initialization

# starting slam
w.run()

# adding lidar data
w.add_lidar_data(
    [1, 2, 3, 4, 5], # distances (m)
    [0.1, 0.2, 0.3, 0.4, 0.5] # angles (rad)
)

# adding imu data
w.add_imu_data(
    1, 2, 3, # linear accelerations (x, y, z) (m/s^2)
    4, 5, 6, # angular velocities (x, y, z) (rad/s)
)

# saving map (now just saves to map.png, still in dev)
w.write_png()

# ending session
w.stop()
w.do_final_optimizations()
```
