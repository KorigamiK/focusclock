cmake_minimum_required(VERSION 3.16)
project(focusclock VERSION 2.1.0)

configure_file(version.h.in version.h)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

if(APPLE)
  # Native Cocoa build: no GTK needed.
  enable_language(OBJCXX)

  add_executable(${PROJECT_NAME}
    macos/main.mm
    utils.cc
  )

  target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
  target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework Cocoa" "-framework CoreText")
else()
  include(FindPkgConfig)
  if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found!" )
  endif()

  pkg_check_modules(gtkmm-4.0 REQUIRED IMPORTED_TARGET gtkmm-4.0)

  option(FOCUSCLOCK_ENABLE_LAYER_SHELL "Enable gtk4-layer-shell integration" ON)
  set(FOCUSCLOCK_HAS_LAYER_SHELL 0)
  if(FOCUSCLOCK_ENABLE_LAYER_SHELL)
    pkg_check_modules(GTK4_LAYER_SHELL QUIET gtk4-layer-shell-0)
    if(GTK4_LAYER_SHELL_FOUND)
      set(FOCUSCLOCK_HAS_LAYER_SHELL 1)
    else()
      message(WARNING
        "gtk4-layer-shell-0 not found; building without layer-shell support")
    endif()
  endif()

  add_executable(${PROJECT_NAME}
    clock.cc
    main.cc
    utils.cc
  )

  target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR})

  target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::gtkmm-4.0)
  if(FOCUSCLOCK_HAS_LAYER_SHELL)
    target_include_directories(${PROJECT_NAME} PRIVATE
      ${GTK4_LAYER_SHELL_INCLUDE_DIRS})
    target_link_directories(${PROJECT_NAME} PRIVATE
      ${GTK4_LAYER_SHELL_LIBRARY_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE
      ${GTK4_LAYER_SHELL_LIBRARIES})
    target_compile_options(${PROJECT_NAME} PRIVATE
      ${GTK4_LAYER_SHELL_CFLAGS_OTHER})
  endif()

  target_compile_definitions(${PROJECT_NAME} PRIVATE
    FOCUSCLOCK_HAS_LAYER_SHELL=${FOCUSCLOCK_HAS_LAYER_SHELL})
endif()

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT ${PROJECT_NAME}
)
