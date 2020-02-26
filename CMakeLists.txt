set(MODULE_NAME Serialization)
set(TARGET_NAME Pargon${MODULE_NAME})
set(MAIN_HEADER Include/Pargon/${MODULE_NAME}.h)

set(PUBLIC_HEADERS
	Include/Pargon/Serialization/BlueprintReader.h
	Include/Pargon/Serialization/BlueprintWriter.h
	Include/Pargon/Serialization/BufferReader.h
	Include/Pargon/Serialization/BufferWriter.h
	Include/Pargon/Serialization/Serialization.h
	Include/Pargon/Serialization/Serializer.h
	Include/Pargon/Serialization/StringReader.h
	Include/Pargon/Serialization/StringWriter.h
)

set(SOURCES
	Source/Core/BlueprintReader.cpp
	Source/Core/BlueprintWriter.cpp
	Source/Core/BufferReader.cpp
	Source/Core/BufferWriter.cpp
	Source/Core/Serialization.cpp
	Source/Core/StringReader.cpp
	Source/Core/StringWriter.cpp
)

set(DEPENDENCIES
	PargonContainers
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Pargon/" FILES ${MAIN_HEADER})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Pargon/${MODULE_NAME}/" PREFIX Include FILES ${PUBLIC_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source/" FILES ${SOURCES})

add_library(${TARGET_NAME} STATIC)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
target_include_directories(${TARGET_NAME} PUBLIC Include)
target_include_directories(${TARGET_NAME} PRIVATE Source)
target_include_directories(${TARGET_NAME} PRIVATE Source/Dependencies/RapidJson)
target_link_libraries(${TARGET_NAME} PUBLIC ${DEPENDENCIES})
target_sources(${TARGET_NAME} PRIVATE "${MAIN_HEADER}" "${PUBLIC_HEADERS}" "${SOURCES}")
