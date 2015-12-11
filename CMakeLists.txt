project(Archeo)
cmake_minimum_required(VERSION 3.0)

find_package(Threads)

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
  company.c
  worker.c
)

add_executable(museum ${COMMON_SOURCES} ${MUSEUM_SOURCES})
target_link_libraries(museum ${CMAKE_THREAD_LIBS_INIT})

add_executable(bank ${COMMON_SOURCES} ${BANK_SOURCES})
target_link_libraries(bank ${CMAKE_THREAD_LIBS_INIT})
