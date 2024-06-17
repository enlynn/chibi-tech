cmake_minimum_required(VERSION 3.27)

function(BuildSample EXAMPLE_NAME)
    SET(SAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
    message(STATUS "Generating project file for example in ${SAMPLE_FOLDER}")

    # Main - what are base headers?
    file(GLOB SOURCES *.cpp ${BASE_HEADERS} ${SAMPLE_FOLDER}/*.cpp)
    file(GLOB HEADERS *.h   ${BASE_HEADERS} ${SAMPLE_FOLDER}/*.h)

    add_executable(${EXAMPLE_NAME} WIN32 ${SOURCES} ${HEADERS})
    target_link_libraries(${EXAMPLE_NAME} chibi-tech)
    target_include_directories(${EXAMPLE_NAME} PRIVATE "../ChibiTech/Source")

    target_include_directories(${EXAMPLE_NAME}
            PRIVATE
            ${SAMPLE_FOLDER} "../Vendor" "../Vendor/AgilitySDK/"
    )

    add_compile_definitions(${EXAMPLE_NAME} PRIVATE ${EXAMPLE_NAME}_CONTENT_PATH="${SAMPLE_FOLDER}/Content")

    IF (WIN32)
        add_compile_definitions(${EXAMPLE_NAME} PRIVATE CT_PLATFORM_WINDOWS)
    endif (WIN32)
endfunction(BuildSample)

function(BuildSamples)
    foreach(SAMPLE ${SAMPLES})
        BuildSample(${SAMPLE})
    endforeach(SAMPLE)
endfunction(BuildSamples)

set(SAMPLES
        HelloTriangle
        HelloCube
        HelloTexture
        CpuRaytracer
)

BuildSamples()