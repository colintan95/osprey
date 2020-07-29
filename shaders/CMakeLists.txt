set(SHADER_SRC_FILES
    "model_frag.frag"
    "model_vert.vert"
    "textured_model_frag.frag"
    "textured_model_vert.vert"
    "triangle_frag.frag"
    "triangle_vert.vert"
)

set(GLSL_COMPILER_PATH "D:\\VulkanSDK\\1.2.141.0\\Bin\\glslc.exe")

set(SHADER_BUILD_FILES)

foreach(file ${SHADER_SRC_FILES})
  string(REGEX REPLACE "(.*).(vert|frag)" "\\1" new_name ${file})

  string(CONCAT new_path ${new_name} ".spv")

  add_custom_command(OUTPUT ${new_path}
    COMMAND cmd /c "${GLSL_COMPILER_PATH}" "${file}" -o "${CMAKE_CURRENT_BINARY_DIR}/${new_path}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    MAIN_DEPENDENCY ${file}
    COMMENT "Creating ${new_path}")

  list(APPEND SHADER_BUILD_FILES ${new_path})
endforeach(file)

add_custom_target(shaders ALL
    COMMAND echo "Add targets for ${SHADER_BUILD_FILES}"
    DEPENDS ${SHADER_BUILD_FILES})