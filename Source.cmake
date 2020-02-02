set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/ForgeApplication.cpp"
   "${SRC_DIR}/ForgeApplication.h"
   "${SRC_DIR}/Main.cpp"

   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Log.h"

   "${SRC_DIR}/Platform/OSUtils.cpp"
   "${SRC_DIR}/Platform/OSUtils.h"
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   target_sources(${PROJECT_NAME} PRIVATE "${SRC_DIR}/Platform/MacOSUtils.mm")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}")

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE "${SRC_DIR}" PREFIX Source FILES ${SOURCE_FILES})
