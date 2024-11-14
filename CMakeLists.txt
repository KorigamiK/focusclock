cmake_minimum_required(VERSION 3.14)
project(focusclock)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
if(NOT PKG_CONFIG_FOUND)
  message(FATAL_ERROR "pkg-config not found!" )
endif()

pkg_check_modules(gtkmm-4.0 REQUIRED IMPORTED_TARGET gtkmm-4.0)
pkg_check_modules(gtk4-layer-shell-0 REQUIRED)

add_executable(${PROJECT_NAME}
  clock.cc
  main.cc
)

target_link_libraries(${PROJECT_NAME} 
  PkgConfig::gtkmm-4.0
  -lgtk4-layer-shell
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
