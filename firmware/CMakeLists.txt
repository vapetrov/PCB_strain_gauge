

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_COMPILER "/usr/bin/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/arm-none-eabi-g++")
set(CMAKE_AR  "/usr/bin/arm-none-eabi-ar")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH   true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD none)

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#set( CMAKE_VERBOSE_MAKEFILE on )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nosys.specs -fstack-protector-strong")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --specs=nosys.specs -fstack-protector-strong -fanalyzer -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding --specs=nosys.specs")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error LANGUAGES C CXX)
if( ipo_supported )
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()
add_compile_options(-Wall)
add_compile_options(-Werror=vla)


# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)
include(FetchContent)



project(strain LANGUAGES C CXX)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project   

add_executable(strain
        strain.c
        )

# pull in common dependencies
target_link_libraries(strain PUBLIC 
                    pico_stdlib 
                    hardware_adc hardware_timer hardware_pwm hardware_i2c)


target_link_options(strain PUBLIC -Wl,--print-memory-usage)

pico_enable_stdio_usb(strain 1)
pico_enable_stdio_uart(strain 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(strain)

# add url via pico_set_program_url
# example_auto_set_url(strain)