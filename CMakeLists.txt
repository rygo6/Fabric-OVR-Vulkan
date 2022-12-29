project(Fabric)

set(TARGET_NAME fabric)
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 17)

file(GLOB SRC_FILES
        src/*.c
        src/*.h
        )

add_executable(${TARGET_NAME}
        ${SRC_FILES}
        )

message(STATUS ">>> FABRIC")

if (WIN32)
    message(STATUS ">>> WIN32")

    add_compile_definitions(WIN32)

    set(VULKAN_SDK_PATH "C:/VulkanSDK/1.3.236.0")
    set(GLFW_SDK_PATH "C:/Developer/glfw-3.3.8.bin.WIN64")
    set(CGLM_SDK_PATH "C:/Developer/cglm")

    target_link_directories(${TARGET_NAME} PUBLIC
            lib
            "${VULKAN_SDK_PATH}/Lib"
            "${GLFW_SDK_PATH}/lib-mingw-w64"
            "${CGLM_SDK_PATH}/build"
            )

    target_include_directories(${TARGET_NAME} PUBLIC
            include
            "${VULKAN_SDK_PATH}/Include"
            "${GLFW_SDK_PATH}/include"
            "${CGLM_SDK_PATH}/include"
            )

    target_link_libraries(${TARGET_NAME}
            glfw3
            gdi32
            vulkan-1
            cglm
            )
endif()

if (UNIX AND NOT APPLE)
    message(STATUS ">>> X11")

    add_compile_definitions(X11)

    find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
    IF (Vulkan_LIBRARY)
        set(Vulkan_FOUND ON)
        MESSAGE("Using bundled Vulkan library version ${Vulkan_LIBRARY}")
    ENDIF()

    target_link_directories(${TARGET_NAME} PUBLIC
            lib
            )

    target_include_directories(${TARGET_NAME} PUBLIC
            include
            )

    target_link_libraries(${TARGET_NAME}
            glfw
            ${Vulkan_LIBRARY}
            cglm
            m
            )
endif()


