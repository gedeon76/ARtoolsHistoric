
# enable modules

# qt includes
include(${QT_USE_FILE})
include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} 
${COIN3D_INCLUDE_DIRS} ${QUARTER_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIR} ${LIVE555_INCLUDE_DIRS}) 
message("live555: "${LIVE555_INCLUDE_DIRS})
# generating UI's
file(GLOB Forms_UIS  RELATIVE ${CMAKE_SOURCE_DIR}/src *.ui)
qt4_wrap_ui(Forms_UIS_H ${Forms_UIS})
qt4_wrap_cpp(SOURCES_MOC ${Forms_UIS_H})


set(ARtools_SRCS ARtools.cpp client.cpp stereovideo.cpp SoStereoTexture.cpp main.cpp ${SOURCES_MOC} ${Forms_UIS})
qt4_automoc(${ARtools_SRCS})
add_executable(ARtools ${ARtools_SRCS})
target_link_libraries(ARtools ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} 
${QT_LIBRARIES} ${COIN3D_LIBRARIES} ${QUARTER_LIBRARIES} ${OPENGL_LIBRARIES} ${FFMPEG_LIBRARIES} ${LIVE555_LIBRARIES})
 