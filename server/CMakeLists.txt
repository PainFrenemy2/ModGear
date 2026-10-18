# ModGear: один код -> редакции (полная / альфа) x платформы (open.mp / SA-MP)
#   -DMODGEAR_PLATFORM=omp|samp   -DMODGEAR_EDITION=full|alfa

set(MODGEAR_PLATFORM "omp" CACHE STRING "omp or samp")
set(MODGEAR_EDITION "full" CACHE STRING "full or alfa")

if(MODGEAR_EDITION STREQUAL "alfa")
	set(MODGEAR_OUTPUT "ModGearAlfa")
else()
	set(MODGEAR_OUTPUT "ModGear")
endif()

project(ModGear-server)

set(CMAKE_CXX_STANDARD 17)

set(MODGEAR_COMMON
	Plugin.cpp
	Events.cpp
	Afk.cpp
	Http.cpp
	Media.cpp
	Paths.cpp
	DebugLog.cpp
	include/lib/InfoServer.cpp
	include/lib/Native.cpp
	include/lib/SimpleParts.cpp
	include/pawn/amxplugin.cpp
	include/raknet/bitstream.cpp
)

if(MODGEAR_PLATFORM STREQUAL "samp")
	set(MODGEAR_PLATFORM_SOURCES
		platform/samp/SampMain.cpp
		platform/samp/SampHooks.cpp
		${CMAKE_SOURCE_DIR}/lib/sampgdk/sampgdk.c
	)
	set(MODGEAR_PLATFORM_INCLUDES ${CMAKE_SOURCE_DIR}/lib/sampgdk)
else()
	set(MODGEAR_PLATFORM_SOURCES platform/omp/OmpMain.cpp)
endif()

# вебкамера AZRTC (свой RakNet-сервер) - только в полной редакции
if(NOT MODGEAR_EDITION STREQUAL "alfa")
	list(APPEND MODGEAR_COMMON AzRtc.cpp)
endif()

add_library(${PROJECT_NAME} SHARED ${MODGEAR_COMMON} ${MODGEAR_PLATFORM_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/include/pawn
	${MODGEAR_PLATFORM_INCLUDES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<STREQUAL:${MODGEAR_EDITION},alfa>:MODGEAR_ALPHA>
	$<$<STREQUAL:${MODGEAR_PLATFORM},omp>:MODGEAR_PLATFORM_OMP>
	$<$<STREQUAL:${MODGEAR_PLATFORM},samp>:MODGEAR_PLATFORM_SAMP SAMPGDK_AMALGAMATION>
	$<$<BOOL:${UNIX}>:LINUX>
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
	# исходники в UTF-8, строки в плагине — в cp1251 (как ждёт клиент)
	target_compile_options(${PROJECT_NAME} PRIVATE /MP $<$<COMPILE_LANGUAGE:CXX>:/source-charset:utf-8 /execution-charset:windows-1251>)
	if(MODGEAR_PLATFORM STREQUAL "samp")
		target_sources(${PROJECT_NAME} PRIVATE platform/samp/plugin.def)
	endif()
else()
	target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-finput-charset=UTF-8 -fexec-charset=CP1251> -fno-strict-aliasing -fvisibility=hidden -fvisibility-inlines-hidden -ffunction-sections -fdata-sections -Wno-format-security -Wno-conversion-null -Wno-pointer-arith)
	target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static-libgcc -Wl,--exclude-libs,ALL -Wl,--gc-sections -s)
	target_link_libraries(${PROJECT_NAME} PRIVATE rt)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${MODGEAR_OUTPUT}")

target_link_libraries(${PROJECT_NAME} PRIVATE mbedtls mbedx509 mbedcrypto)

if(NOT MODGEAR_EDITION STREQUAL "alfa")
	target_link_libraries(${PROJECT_NAME} PRIVATE raknet-azrtc)
	set_source_files_properties(AzRtc.cpp PROPERTIES
		INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/lib/raknet;${CMAKE_SOURCE_DIR}/lib/raknet/Include/raknet"
		COMPILE_DEFINITIONS "RAKNET_LEGACY;PACKETS_PER_TICK=3")
endif()

if(MODGEAR_PLATFORM STREQUAL "omp")
	target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK OMP-Network)
endif()

