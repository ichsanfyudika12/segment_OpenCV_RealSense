## 🚀 How to Use

### 🛠 Prerequisites

#### ✅ Install OpenCV (with Contrib modules)
```bash
sudo apt update && sudo apt install -y cmake g++ libgtk-3-dev libtbb-dev
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
cd opencv && mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D OPENCV_EXTRA_MODULES_PATH=~/opencv_contrib/modules ..
make -j$(nproc) && sudo make install

🔗 OpenCV GitHub
🔗 OpenCV Contrib GitHub
✅ Install Intel RealSense SDK

Intel RealSense: 📖 RealSense Installation Guide

