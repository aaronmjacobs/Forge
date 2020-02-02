set(SRC_DIR "${PROJECT_SOURCE_DIR}/Source")

target_sources(${PROJECT_NAME} PRIVATE
   "${SRC_DIR}/ForgeApplication.cpp"
   "${SRC_DIR}/ForgeApplication.h"
   "${SRC_DIR}/Main.cpp"

   "${SRC_DIR}/Core/Assert.h"
   "${SRC_DIR}/Core/Log.cpp"
   "${SRC_DIR}/Core/Log.h"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${SRC_DIR}")

get_target_property(SOURCE_FILES ${PROJECT_NAME} SOURCES)
source_group(TREE "${SRC_DIR}" PREFIX Source FILES ${SOURCE_FILES})
