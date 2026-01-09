# 安装 Conan 依赖
conan install . --build=missing

# 进入构建目录并使用 CMake 配置
cd build
cmake .. --preset conan-release

# 进入 Release 目录并编译
cd Release
make