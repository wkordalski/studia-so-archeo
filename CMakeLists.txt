project(Archeo)
cmake_minimum_required(VERSION 2.8)

find_package(Threads)

add_definitions(-std=c99 -D_XOPEN_SOURCE)

set(COMMON_SOURCES
  common.c
  double_queue.c
  util.c
)

set(MUSEUM_SOURCES
  museum.c
)

set(BANK_SOURCES
  bank.c
)

set(COMPANY_SOURCES
  company.c
  worker.c
)

add_library(common STATIC ${COMMON_SOURCES})
target_link_libraries(${CMAKE_THREAD_LIBS_INIT})

add_executable(museum ${MUSEUM_SOURCES})
target_link_libraries(museum common ${CMAKE_THREAD_LIBS_INIT})

add_executable(bank ${BANK_SOURCES})
target_link_libraries(bank common ${CMAKE_THREAD_LIBS_INIT})

add_executable(company ${COMPANY_SOURCES})
target_link_libraries(company common ${CMAKE_THREAD_LIBS_INIT})
