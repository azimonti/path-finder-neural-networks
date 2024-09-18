cmake_minimum_required(VERSION 3.13.4)
project(path-finder)
set (PROJECT_VERSION "1.0"    )
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION})
# location of ma-libs
set(MA_LIBS_ROOT "${CMAKE_SOURCE_DIR}/externals/ma-libs")
set(MA_LIBS_CPP_ROOT "${MA_LIBS_ROOT}/cpp")
list(APPEND CMAKE_MODULE_PATH "${MA_LIBS_ROOT}/cmake_modules")
include( COMPILERCOMMON     )
include( COMPILERCPP        )
include( FindLIBS           )
cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)
set(CMAKE_CXX_STANDARD 17   )

if(MSVC)
    generic_libs_find(glew ON       )
    generic_libs_find(glfw ON       )
    generic_libs_find(glm  ON       )
else()
    generic_libs_find(glew OFF      )
    generic_libs_find(glfw OFF      )
    generic_libs_find(glm  OFF      )
endif()
generic_libs_find(imgui ON          )
generic_libs_find(implot ON         )
generic_libs_find(imGuIZMO.quat ON  )

include_directories( ${IMGUI_INCLUDE_DIRS}           )
include_directories( ${IMGUI_INCLUDE_DIRS}/backends  )
include_directories( ${IMGUI_INCLUDE_DIRS}/misc/cpp  )
include_directories( ${GLM_INCLUDE_DIRS}             )
include_directories( ${GLEW_INCLUDE_DIRS}            )
include_directories( ${GLEW_INCLUDE_DIRS}/include    )
include_directories( ${GLFW_INCLUDE_DIRS}            )
include_directories( ${GLFW_INCLUDE_DIRS}/include    )
include_directories( ${IMGUIZMOQUAT_INCLUDE_DIRS}/imGuIZMO.quat )
include_directories( ${IMGUI_INCLUDE_DIRS}/..        )
include_directories( ${IMPLOT_INCLUDE_DIRS}          )
include_directories( ${MA_LIBS_CPP_ROOT}                )
include_directories( ${MA_LIBS_CPP_ROOT}/math           )
include_directories( ${MA_LIBS_CPP_ROOT}/libnn/src      )
include_directories( ${MA_LIBS_CPP_ROOT}/libgraphic_engine/src      )
include_directories( ${MA_LIBS_CPP_ROOT}/utils          )

add_definitions( -DVGIZMO_USES_GLM -DLOGGING )
add_definitions( -DGLEW_STATIC -DGLM_ENABLE_EXPERIMENTAL )
if (MSVC)
    add_compile_options(/w34389 /w34245 /w34365 /w34388) # Warning about signed/unsigned
else()
    #add_compile_options(-Wall -Wextra -pedantic -Wconversion -Wno-float-conversion -Wno-unused-variable -Wno-unused-parameter -Wno-unused-private-field -Wno-unused-but-set-variable)
endif()

# === HDF5
if(MSVC)
    generic_libs_find(hdf5 ON       )
    generic_libs_find(zlib ON       )
    generic_libs_find(png  ON       )
    include_directories( ${HDF5_INCLUDE_DIRS}/src )
    include_directories( ${HDF5_INCLUDE_DIRS2} )
    include_directories( ${HDF5_INCLUDE_DIRS2}/hdf5 )
    link_directories( ${ZLIB_LIBRARY_PATH}     )
else()
    generic_libs_find(hdf5 OFF      )
    generic_libs_find(png  OFF      )
endif()
include_directories( ${HDF5_INCLUDE_DIRS} )
include_directories( ./utils )
link_directories( ${HDF5_LIBRARY_PATH}  )

file( GLOB SRCS         "src/*.cpp"                     "*.h"          )
set( SRCS_LOG ${MA_LIBS_CPP_ROOT}/utils/log/log.cpp )

link_directories(${LIBS_DIR}          )
link_directories(${GLEW_LIBRARY_PATH} )
link_directories(${GLEW_LIBRARY_PATH2})
link_directories(${GLFW_LIBRARY_PATH} )
link_directories( ${MA_LIBS_ROOT}/build/${CMAKE_BUILD_TYPE} )


add_executable( ${PROJECT_NAME} ${SRCS} ${SRCS_LOG} )

if(UNITYBUILD)
    set_property( TARGET ${PROJECT_NAME} PROPERTY UNITY_BUILD ON )
endif()

if(MSVC)
    target_link_libraries( ${PROJECT_NAME} debug libhdf5_D debug zlibstaticd debug libpng16d optimized libhdf5 optimized zlibstatic optimized libpng16  )
else()
    target_link_libraries( ${PROJECT_NAME} hdf5 png )
endif()

if (MSVC)
    target_link_libraries( ${PROJECT_NAME} imgui imguizmoquat implot ${OPENGL_gl_LIBRARY} glew glfw3 debug cpp_graphic_engined debug cpp_nnd optimized cpp_graphic_engine optimized cpp_nn )
elseif(APPLE)
    target_link_libraries( ${PROJECT_NAME} libimgui.a imguizmoquat implot ${OPENGL_gl_LIBRARY} libglfw.dylib glew "-framework Foundation -framework IOKit -framework Cocoa" debug cpp_graphic_engined debug cpp_nnd optimized cpp_graphic_engine optimized cpp_nn )
else()
    target_link_libraries( ${PROJECT_NAME} libimgui.a imguizmoquat implot ${OPENGL_gl_LIBRARY} libglfw.so libGLEW.so -ldl -lpthread -lGL -lGLU debug cpp_graphic_engined debug cpp_nnd optimized cpp_graphic_engine optimized cpp_nn )
endif()

if (MSVC)
    macro(COPYDLL)
      if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../../Release/
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/externals/ma-libs/build/Release/cpp_alg_lapack.dll
          ${CMAKE_BINARY_DIR}/../../Release/
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/externals/ma-libs/build/Release/cpp_nn.dll
          ${CMAKE_BINARY_DIR}/../../Release/
          )
      elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../../Debug/
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/externals/ma-libs/build/Debug/cpp_alg_lapackd.dll
          ${CMAKE_BINARY_DIR}/../../Debug/
          COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/externals/ma-libs/build/Debug/cpp_nnd.dll
          ${CMAKE_BINARY_DIR}/../../Debug/
          )
      endif()
      ADD_CUSTOM_COMMAND( TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<$<CONFIG:Release>:${LOCAL_DIR}/bin/libpng16.dll>
        $<$<CONFIG:Debug>:${LOCAL_DIR}/bin/libpng16d.dll>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
      ADD_CUSTOM_COMMAND( TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<$<CONFIG:Release>:${LOCAL_DIR}/bin/zlib.dll>
        $<$<CONFIG:Debug>:${LOCAL_DIR}/bin/zlibd.dll>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    endmacro()
    cmake_language(CALL COPYDLL)
endif()
