set(SHADER_DIR "${PROJECT_SOURCE_DIR}/Shaders")
set(SHADER_BIN_DIR "${RES_DIR}/Shaders")

set(SHADER_HEADER_FILES
   "${SHADER_DIR}/Lighting.glsl"
   "${SHADER_DIR}/Masked.glsl"
   "${SHADER_DIR}/RenderQuality.glsl"
   "${SHADER_DIR}/View.glsl"
)

set(SHADER_SOURCE_FILES
   "${SHADER_DIR}/BloomDownsample.frag"
   "${SHADER_DIR}/BloomUpsample.frag"
   "${SHADER_DIR}/Composite.frag"
   "${SHADER_DIR}/Depth.vert"
   "${SHADER_DIR}/DepthMasked.frag"
   "${SHADER_DIR}/DepthMasked.vert"
   "${SHADER_DIR}/Forward.frag"
   "${SHADER_DIR}/Forward.vert"
   "${SHADER_DIR}/Normal.frag"
   "${SHADER_DIR}/Normal.vert"
   "${SHADER_DIR}/Tonemap.frag"
   "${SHADER_DIR}/Screen.vert"
   "${SHADER_DIR}/Skybox.frag"
   "${SHADER_DIR}/SSAO.frag"
   "${SHADER_DIR}/SSAOBlur.frag"
   "${SHADER_DIR}/SSRApply.frag"
   "${SHADER_DIR}/SSRBlur.frag"
   "${SHADER_DIR}/SSRGenerate.frag"
)

find_program(GLSLC glslc)
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

set(SHADER_BINARY_FILES)
foreach(SHADER_SOURCE_FILE ${SHADER_SOURCE_FILES})
   get_filename_component(SHADER_SOURCE_FILE_NAME ${SHADER_SOURCE_FILE} NAME)
   set(SHADER_BINARY_FILE "${SHADER_BIN_DIR}/${SHADER_SOURCE_FILE_NAME}.spv")
   list(APPEND SHADER_BINARY_FILES ${SHADER_BINARY_FILE})

   add_custom_command(
      OUTPUT ${SHADER_BINARY_FILE}
      COMMAND ${GLSLC} -o ${SHADER_BINARY_FILE} ${SHADER_SOURCE_FILE}
      DEPENDS ${SHADER_SOURCE_FILE} ${SHADER_HEADER_FILES}
      IMPLICIT_DEPENDS CXX ${SHADER_SOURCE_FILE}
      COMMENT "Compiling shader ${SHADER_BINARY_FILE} from ${SHADER_SOURCE_FILE}"
      VERBATIM
   )
endforeach()

set(SHADERS_TARGET_NAME "${PROJECT_NAME}-Shaders")
add_custom_target(
   ${SHADERS_TARGET_NAME}
   DEPENDS ${SHADER_BINARY_FILES}
   SOURCES ${SHADER_HEADER_FILES} ${SHADER_SOURCE_FILES}
)
source_group(TREE "${SHADER_DIR}" PREFIX Shaders FILES ${SHADER_HEADER_FILES} ${SHADER_SOURCE_FILES})
add_dependencies(${PROJECT_NAME} ${SHADERS_TARGET_NAME})
