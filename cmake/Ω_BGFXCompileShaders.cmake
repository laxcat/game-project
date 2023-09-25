# #
# # Compiles BGFX shaders as part of build process
# #
# # Output:
# # BGFXShader_target
# # Use this target to add the compiled shaders as dependencies of another target. (name can be configured)
# #
# function(BGFXCompileShaders)
#     set(options
#         embed               # should prep shaders for embedding. if set, removes requirement for output dir.
#     )
#     set(oneValueArgs
#         output_dir          # required if embed not set
#         shaderc             # path to bgfx shader compiler binary "shaderc"
#         target              # target variable. target pointed to will depend on all outputs
#     )
#     set(multiValueArgs
#         shader_dirs         # list of shader directories
#         profile_list        # list of api profiles (see shaderc help for full list)
#         include_dirs        # list of shader includes directories. bgfx/src is automatically added later if possible.
#     )
#     cmake_parse_arguments(args
#         "${options}"
#         "${oneValueArgs}"
#         "${multiValueArgs}"
#         ${ARGN}
#     )

#     # get bgfx setup properties
#     FetchContent_GetProperties(bgfx)

#     # target
#     set(target ${args_target})
#     if(NOT target)
#         set(target BGFXShader_target)
#     endif()

#     # shaderc bin location
#     # arg was set, make sure it exists
#     if(args_shaderc AND NOT EXISTS "${args_shaderc}")
#         message(FATAL_ERROR "Provided path to shaderc does not exist (${args_shaderc}). Aborting shader compile.")
#     endif()
#     # use arg as default (but arg might not have been set)
#     set(shaderc "${args_shaderc}")
#     # use BGFX path if we can see that var
#     if((NOT shaderc) AND SetupLib_BGFX_shaderc_PATH AND EXISTS "${SetupLib_BGFX_shaderc_PATH}")
#         set(shaderc "${SetupLib_BGFX_shaderc_PATH}")
#     endif()
#     # use best guess
#     set(shaderc_best_guess "${CMAKE_BINARY_DIR}/engine/shaderc")
#     if((NOT shaderc) AND EXISTS "${shaderc_best_guess}")
#         set(shaderc ${shaderc_best_guess})
#     endif()
#     # could not set from any possibility
#     if(NOT shaderc)
#         message(FATAL_ERROR "Could not find shaderc. Aborting shader compile.")
#     else()
#         message(STATUS "Using ${shaderc}")
#     endif()

#     # setup list of include directories
#     set(include_dirs "${args_include_dirs}")
#     if(bgfx_SOURCE_DIR AND EXISTS "${bgfx_SOURCE_DIR}/bgfx/src")
#         list(APPEND include_dirs "${bgfx_SOURCE_DIR}/bgfx/src")
#     endif()
#     set(include_dirs_args "")
#     foreach(include_dir ${include_dirs})
#         # check existence of include directories
#         if(NOT EXISTS "${include_dir}")
#             message(WARNING "Could not find provided include directory \"${include_dir}\".")
#         endif()
#         # create arg string
#         string(APPEND include_dirs_args "-i;${include_dir};")
#     endforeach()

#     # check that output directory was provided (if required)
#     # not required if embed is set. required if embed not set.
#     # don't check if it exists, there is a rule to create it later.
#     if((NOT ${args_embed}) AND (NOT args_output_dir))
#         message(FATAL_ERROR "Output directory must be provided if embed is not set (embed=${args_embed}). Aborting shader compile.")
#     endif()

#     # warn if not inputs or profiles
#     if(NOT args_profile_list)
#         message(WARNING "Shader profile list empty. Nothing to do.")
#     endif()
#     if(NOT args_shader_dirs)
#         message(WARNING "No input directories provided. Nothing to do.")
#     endif()

#     # vars
#     set(outputs)

#     # this macro gets run for each frag/vert/comp, for each shader, for each profile
#     macro(compile
#         shader_dir  # path to directory containing shaders
#         type        # v/f/c
#         profile     # see shaderc help
#         output_dir  # output directory
#     )
#         cmake_path(GET shader_dir FILENAME name)

#         set(output ${output_dir}/${type}s_${name}.bin)
#         set(input ${shader_dir}/${type}s_${name}.sc)
#         set(varying ${shader_dir}/varying.def.sc)

#         if(EXISTS "${input}")
#             # easier to read output using relative paths
#             set(input_display ${input})
#             set(output_display ${output})
#             if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
#                 cmake_path(RELATIVE_PATH input BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE input_display)
#                 cmake_path(RELATIVE_PATH output BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE output_display)
#             endif()

#             # build shader
#             add_custom_command(
#                 OUTPUT  ${output}
#                 DEPENDS ${shaderc} ${input} ${varying} "${output_dir}"
#                 COMMAND ${shaderc}
#                         --platform osx
#                         -p ${profile}
#                         --type ${type}
#                         -f ${input}
#                         -o ${output}
#                         "${include_dirs_args}"
#                 COMMAND_EXPAND_LISTS
#                 VERBATIM
#                 COMMENT "Compiling ${type} shader ${input_display} for ${profile} to ${output_display}"
#             )
#             # embed only
#             if(${args_embed})
#                 set(output_final "${shader_dir}/${type}s_${name}.${profile}.bin.geninc")
#                 set(output_final_display ${output_final})
#                 if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
#                     cmake_path(RELATIVE_PATH output_final BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE output_final_display)
#                 endif()
#                 add_custom_command(
#                     OUTPUT  ${output_final}
#                     DEPENDS ${output}
#                     WORKING_DIRECTORY "${output_dir}"
#                     COMMAND "xxd"
#                             -i "${type}s_${name}.bin"
#                             > ${output_final}
#                     COMMAND_EXPAND_LISTS
#                     VERBATIM
#                     COMMENT "Converting shader ${output_display} to include ${output_final_display}"
#                 )
#                 # add output to output list
#                 list(APPEND outputs ${output_final})
#             else()
#                 # add output to output list
#                 list(APPEND outputs ${output})
#             endif()
#         endif()
#     endmacro()

#     # for each output profile
#     foreach(profile ${args_profile_list})
#         # determine output directory
#         if("${profile}" MATCHES "^(120|130|140|150|330|400|410|420|430|440)$")
#             set(api_name "glsl")
#         elseif("${profile}" MATCHES "^(100_es|300_es|310_es|320_es)$")
#             set(api_name "essl")
#         elseif("${profile}" MATCHES "metal")
#             set(api_name "metal")
#         # TODO: add direct3d detection for windows builds
#         else()
#             message(FATAL_ERROR "Uknown profile \"${profile}\". See shaderc help for profile list.")
#         endif()

#         # set ouput directory
#         # embed only
#         if(${args_embed})
#             set(output_dir "${CMAKE_BINARY_DIR}/temp_embed_${target}/${api_name}")
#         # if NOT embed...
#         else()
#             set(output_dir "${args_output_dir}/${api_name}")
#         endif()

#         # create rule to make output directory
#         add_custom_command(
#             OUTPUT  "${output_dir}"
#             COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
#             VERBATIM
#             COMMENT "Creating directory for shader output: ${output_dir}"
#         )

#         # for each shader input directory
#         foreach(shader_dir ${args_shader_dirs})
#             if(NOT EXISTS "${shader_dir}")
#                 message(WARNING "Could not find provided shader directory \"${shader_dir}\".")
#                 continue()
#             endif()
#             compile(${shader_dir} v ${profile} ${output_dir})
#             compile(${shader_dir} f ${profile} ${output_dir})
#             compile(${shader_dir} c ${profile} ${output_dir})
#         endforeach()
#     endforeach()

#     # create/append to custom target
#     if (TARGET ${target})
#         add_dependencies(${target} ${output_final})
#     else()
#         add_custom_target(${target} DEPENDS ${outputs})
#     endif()

# endfunction()
