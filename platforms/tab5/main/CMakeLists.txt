file(GLOB_RECURSE APP_LAYER_SRCS
    ./app_main.cpp
    ../../../app/*.c
    ../../../app/*.cc
    ../../../app/*.cpp
)

# "." .nlohmann_json is ignored during scan
set(APP_LAYER_INCS
    ../../../app
    ../../../dependencies/.nlohmann_json/single_include
)

file(GLOB_RECURSE MY_HAL_SRCS
    ./hal/*.c
    ./hal/*.cc
    ./hal/*.cpp
)

file(GLOB SOUND_ASSETS
    ../../../app/assets/audio/*.mp3
)


#target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable
#	-Wno-missing-field-initializers
#        -Wno-format)
# message(STATUS "file list :" ${app_sources})


idf_component_register(SRCS ${APP_LAYER_SRCS} ${MY_HAL_SRCS}
                    INCLUDE_DIRS "." ${APP_LAYER_INCS}
                    EMBED_FILES ${SOUND_ASSETS})


