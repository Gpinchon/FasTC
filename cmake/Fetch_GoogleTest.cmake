include(FetchContent)

# Fetch GOOGLE_TEST
macro(Fetch_GoogleTest)
  if (NOT TARGET gtest)
    FetchContent_Declare(
        GTEST
        GIT_REPOSITORY  https://github.com/google/googletest.git
        GIT_TAG         v1.15.2
    )
    FetchContent_MakeAvailable(GTEST)
  endif (NOT TARGET gtest)
endmacro()
