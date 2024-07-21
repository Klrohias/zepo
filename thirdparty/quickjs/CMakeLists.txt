include(FetchContent)

FetchContent_Declare(
        quickjs-universal
        GIT_REPOSITORY https://github.com/Klrohias/quickjs-universal
        GIT_TAG ed777d81325281fdd10d207c2892266a88ec34e4
)

FetchContent_MakeAvailable(quickjs-universal)