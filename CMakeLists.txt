cmake_minimum_required(VERSION 3.16)
project(focusclock VERSION 2.1.0)

configure_file(version.h.in version.h)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

if(APPLE)
  # Native Cocoa build.
  enable_language(OBJCXX)

  add_executable(${PROJECT_NAME}
    macos/main.mm
    options.cc
    utils.cc
  )

  target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework Cocoa" "-framework CoreText")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Native Wayland (wlr-layer-shell) build.
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
  pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
  pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
  pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
  pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)
  if(NOT WAYLAND_SCANNER)
    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
  endif()

  # layer-shell's get_popup references xdg_popup, so xdg-shell is needed too.
  set(PROTOCOL_SOURCES)
  foreach(xml
      ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
      ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml)
    get_filename_component(name ${xml} NAME_WE)
    set(header ${CMAKE_BINARY_DIR}/${name}-client-protocol.h)
    set(code ${CMAKE_BINARY_DIR}/${name}-protocol.c)
    add_custom_command(
      OUTPUT ${header} ${code}
      COMMAND ${WAYLAND_SCANNER} client-header ${xml} ${header}
      COMMAND ${WAYLAND_SCANNER} private-code ${xml} ${code}
      DEPENDS ${xml}
      VERBATIM)
    list(APPEND PROTOCOL_SOURCES ${header} ${code})
  endforeach()

  add_executable(${PROJECT_NAME}
    linux/main.cc
    options.cc
    utils.cc
    ${PROTOCOL_SOURCES}
  )

  target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::WAYLAND_CLIENT PkgConfig::CAIRO)
else()
  message(FATAL_ERROR "focusclock supports macOS and Linux (Wayland) only")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT ${PROJECT_NAME}
)
