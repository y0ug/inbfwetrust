cmake_minimum_required(VERSION 3.10)

# set the project name
project(inbfwetrust)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -msse -msse2 -msse3 -maes")

# add the executable
add_executable(croco main_croco.c bf.c crypto/aes-ni.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(croco PRIVATE Threads::Threads m)
if(WIN32)
	target_link_libraries(croco PRIVATE -static)
endif()
