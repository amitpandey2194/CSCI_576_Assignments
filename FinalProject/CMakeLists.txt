cmake_minimum_required(VERSION 2.8.12)

PROJECT(GAZED_BASED_COMPRESSION_VR_BITSTREAM_DECODER)

find_package(OpenCV REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS})
file(GLOB SOURCES
	"*.cpp"
)
add_executable(GAZE_BASED_COMP_DECOMP ${SOURCES})
target_link_libraries(GAZE_BASED_COMP_DECOMP ${OpenCV_LIBS})