# Installing CartoPy

For installing package you first need to build a wheel. Building requires pre-compiled Google Cartographer:

If you don`t have Cartographer, you can easily build it on Ubuntu/Debian:
```bash
sudo apt-get install -y \
    google-mock libceres-dev liblua5.3-dev \
    libprotobuf-dev protobuf-compiler \
    libboost-all-dev libgflags-dev \
    libgoogle-glog-dev libatlas-base-dev \
    libeigen3-dev cmake ninja-build \
    libunwind-dev libabsl-dev libcairo-dev

cd cartographer
mkdir build && cd build
cmaeke .. -G Ninja
sudo ninja
sudo ninja install
```

It may take a few attempts to compile. You can ask for help compiling Cartographer at `vadimlebedenko0@gmail.com`

Next create a venv (you`ll probably need it for installing package after):
```bash
sudo apt-get install python3-build python3-venv # install venv and build if not installed
python3 -m venv venv
```

Run these commands:
```bash
venv/bin/python3 -m build
venv/bin/pip install dist/*.whl
```

Package is now installed in your venv. Example using:
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