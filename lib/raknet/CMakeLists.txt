# RakNet 2.52 из open.mp (github.com/openmultiplayer/RakNet, 64d8ddb) - для второго сервера вебкамеры AZRTC.
# Правки ModGear: query.hpp/ICore заменены на rakshim.hpp, приём разбитых (split) пакетов от клиента включён.
project(raknet-azrtc CXX)

set(RAKNET_SOURCES
	Source/BitStream.cpp
	Source/CheckSum.cpp
	Source/CommandParserInterface.cpp
	Source/ConsoleServer.cpp
	Source/DataBlockEncryptor.cpp
	Source/DS_HuffmanEncodingTree.cpp
	Source/GetTime.cpp
	Source/InternalPacketPool.cpp
	Source/NetworkTypes.cpp
	Source/RakClient.cpp
	Source/RakNetCommandParser.cpp
	Source/RakNetworkFactory.cpp
	Source/RakPeer.cpp
	Source/rakserver.cpp
	Source/RakSleep.cpp
	Source/Rand.cpp
	Source/ReliabilityLayer.cpp
	Source/rijndael.cpp
	Source/RPCMap.cpp
	Source/SHA1.cpp
	Source/SimpleMutex.cpp
	Source/SocketLayer.cpp
	Source/StringCompressor.cpp
	Source/StringTable.cpp
	Source/PluginInterface.cpp
	SAMPRakNet.cpp
)

add_library(raknet-azrtc STATIC ${RAKNET_SOURCES})
set_property(TARGET raknet-azrtc PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET raknet-azrtc PROPERTY CXX_STANDARD 17)

# пути и define не PUBLIC: в плагине свой raknet/bitstream.h, на Windows (без учёта регистра) они бы спутались.
# AzRtc.cpp подключает их себе сам (server/CMakeLists.txt)
target_compile_definitions(raknet-azrtc PRIVATE RAKNET_LEGACY PACKETS_PER_TICK=3)
target_include_directories(raknet-azrtc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Include/raknet)

if(WIN32)
	target_compile_definitions(raknet-azrtc PRIVATE _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
	target_link_libraries(raknet-azrtc PUBLIC Ws2_32)
else()
	target_compile_definitions(raknet-azrtc PRIVATE __stdcall=)
	target_compile_options(raknet-azrtc PRIVATE -w -fvisibility=hidden -fvisibility-inlines-hidden)
	find_package(Threads REQUIRED)
	target_link_libraries(raknet-azrtc PUBLIC Threads::Threads)
endif()
