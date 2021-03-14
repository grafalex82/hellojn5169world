SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_TOOLCHAIN_FILE JN5169.cmake)

PROJECT(GPSLogger)
CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
ENABLE_LANGUAGE(CXX)

IF(NOT SDK_PREFIX)
     MESSAGE(FATAL_ERROR "No SDK_PREFIX specified")
ENDIF()


################################
# Common settings

ADD_DEFINITIONS(
	-DJENNIC_CHIP_NAME=_JN5169
	-DJENNIC_CHIP_FAMILY_NAME=_JN516x
	-DJENNIC_CHIP_FAMILY_JN516x
	-DJENNIC_CHIP_FAMILY=JN516x
	-DJN516x=5160
	-DDEBUG_ZTIMER
	-DDBG_ENABLE
)


################################
# Zigbee Library

SET(ZIGBEE_SRC
	${SDK_PREFIX}/Components/ZigbeeCommon/Source/ZTimer.c
)
ADD_LIBRARY(ZigBee STATIC ${ZIGBEE_SRC})
TARGET_INCLUDE_DIRECTORIES(ZigBee PRIVATE
	${SDK_PREFIX}/Components/PWRM/Include
)


################################
# Hello World

INCLUDE_DIRECTORIES(
	${SDK_PREFIX}/Components/Common/Include
	${SDK_PREFIX}/Components/Dbg/Include
	${SDK_PREFIX}/Components/HardwareAPI/Include
	${SDK_PREFIX}/Components/ZigbeeCommon/Include
)

LINK_DIRECTORIES(
	${SDK_PREFIX}/Components/Library
	${SDK_PREFIX}/Chip/JN5169/Build
	${SDK_PREFIX}/Platform/DK4/Library
)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -TAppBuildZBPro.ld")

SET(SOURCES
	HelloWorld.c
)

ADD_EXECUTABLE(HelloWorld ${SOURCES})
TARGET_LINK_LIBRARIES(HelloWorld
	ZigBee	

        -Wl,--start-group

        PWRM_JN516x
	HardwareApi_JN5169
	DBG_JN516x
	Boot_JN516x

        -Wl,--end-group
)

STM32_PRINT_SIZE_OF_TARGETS(HelloWorld)
STM32_ADD_HEX_BIN_TARGETS(HelloWorld)
STM32_ADD_DUMP_TARGET(HelloWorld)
