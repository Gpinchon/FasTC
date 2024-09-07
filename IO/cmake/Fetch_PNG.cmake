include(FetchContent)

macro(Fetch_PNG)
  find_package(PNG QUIET)
  if(NOT PNG_FOUND)
    #if we don't have it already fetch it and install it in build directory
    FetchContent_Declare(
      PNG
      GIT_REPOSITORY https://github.com/glennrp/libpng.git
      GIT_TAG        v1.6.37
    )
    FetchContent_GetProperties(PNG)
    if (NOT png_POPULATED)
      FetchContent_Populate(PNG)
    endif (NOT png_POPULATED)
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        "-DCMAKE_BUILD_TYPE=Release"
        -G ${CMAKE_GENERATOR}
        -S ${png_SOURCE_DIR}
        -B ${png_BINARY_DIR}
        --install-prefix ${CMAKE_BINARY_DIR}/external)
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        --build ${png_BINARY_DIR}
        --config Release)
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        --install ${png_BINARY_DIR}
        --prefix ${CMAKE_BINARY_DIR}/external
        --config Release)
    find_package(PNG REQUIRED)
  endif(NOT PNG_FOUND)
endmacro()
