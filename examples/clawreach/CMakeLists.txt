cmake_minimum_required(VERSION 3.5)

# ClawReach - Physical AI Presence for OpenClaw
# Streams audio + video to a self-hosted server

set(EXTRA_COMPONENT_DIRS "src" "components/esp-libopus")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_compile_options(-fdiagnostics-color=always -Wno-error=format= -Wno-format)
project(clawreach)
