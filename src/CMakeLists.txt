# 查找必要的依赖包
find_package(OpenCV REQUIRED)

# 设置编译选项
set(PRJ_COMPILE_OPTIONS "")

set(MIC_NUM 6)

# 收集源文件
file(GLOB_RECURSE SRC ${CMAKE_CURRENT_LIST_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/*.c)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SRC})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_LIST_DIR}/../include
  ${CMAKE_CURRENT_LIST_DIR}/utils/jsoncpp
  ${CMAKE_CURRENT_LIST_DIR}
)

# 设置编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
  "-DMIC_NUM=${MIC_NUM}"
)

target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib/arm)

# 链接库
target_link_libraries(${PROJECT_NAME}
  ${OpenCV_LIBS}
  udev
  portaudio
  avvtn_mic${MIC_NUM}
  aiui
  pthread
)
