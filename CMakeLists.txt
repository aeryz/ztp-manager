cmake_minimum_required(VERSION 3.13)
project(ZTP-manager)

set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED system)
find_package(libmongocxx REQUIRED)
find_package(libbsoncxx REQUIRED)
include_directories(${BOOST_INCLUDE_DIRS})
include_directories(${LIBMONGOCXX_INCLUDE_DIRS})
include_directories(${LIBBSONCXX_INCLUDE_DIRS})

add_executable(ztp-manager manager/main.cpp manager/server.cpp manager/server.h manager/manager.cpp manager/manager.h helper/database_ctrl.cpp helper/database_ctrl.h helper/data_types.h external/easylogging++/easylogging++.cc)

target_link_libraries(ztp-manager ${Boost_LIBRARIES})
#target_link_libraries(ztp-manager redox)
#target_link_libraries(ztp-manager ev)
#target_link_libraries(ztp-manager hiredis)
target_link_libraries(ztp-manager ${LIBMONGOCXX_LIBRARIES})
target_link_libraries(ztp-manager ${LIBBSONCXX_LIBRARIES})
target_link_libraries(ztp-manager Threads::Threads)

install(TARGETS ztp-manager DESTINATION /usr/local/bin)

IF (NOT EXISTS /var/log/ztp-manager)
    file(MAKE_DIRECTORY /var/log/ztp-manager)
ENDIF()

IF (NOT EXISTS /etc/ztp/ztp-manager)
    file(MAKE_DIRECTORY /etc/ztp/ztp-manager)
ENDIF()

install(FILES "${PROJECT_BINARY_DIR}/../defaults/ztp-manager-logging.conf" DESTINATION /etc/ztp)