# cmake_minimum_required(VERSION 3.5.0)
# project(FTK VERSION 0.1.0 LANGUAGES C CXX)

# # Imposta il supporto per C++17
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)


# # Trova OpenCV
# find_package(OpenCV REQUIRED)
# if (OpenCV_FOUND)
#     message(STATUS "OpenCV trovato: ${OpenCV_VERSION}")
#     include_directories(${OpenCV_INCLUDE_DIRS})
# else()
#     message(FATAL_ERROR "OpenCV non trovato. Assicurati che sia installato correttamente.")
# endif()


# # Aggiungi eseguibili
# add_executable(simul_multi_file src/simul_multi_file.cpp)

# # include locali
# target_include_directories(simul_multi_file PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Linka librerie
# target_link_libraries(simul_multi_file 
#     ${OpenCV_LIBS} 
# )


cmake_minimum_required(VERSION 3.21)
project(FTK LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sorgenti
file(GLOB SRC "src/*.cpp")
add_executable(simul_multi_file ${SRC})
target_include_directories(simul_multi_file PRIVATE ${CMAKE_SOURCE_DIR}/include)

# OpenCV solo sulla macchina di build
find_package(OpenCV REQUIRED)
if (TARGET opencv_world)
  target_link_libraries(simul_multi_file PRIVATE opencv_world)
else()
  target_link_libraries(simul_multi_file PRIVATE ${OpenCV_LIBS})
endif()

# Opzionale: warning utili su MSVC
if (MSVC)
  target_compile_options(simul_multi_file PRIVATE /W4 /permissive-)
endif()

# ---- Copia DLL OpenCV accanto all'exe (Windows) ----
# Imposta questo path alla tua cartella bin di OpenCV
set(OPENCV_BIN_DIR "C:/opencv/build/x64/vc16/bin" CACHE PATH "Cartella con le DLL di OpenCV")

if (WIN32 AND OPENCV_BIN_DIR)
  file(GLOB _cv_world "${OPENCV_BIN_DIR}/opencv_world*.dll")
  file(GLOB _cv_ffmpeg "${OPENCV_BIN_DIR}/opencv_videoio_ffmpeg*.dll")
  file(GLOB _cv_core   "${OPENCV_BIN_DIR}/opencv_core*.dll")
  file(GLOB _cv_img    "${OPENCV_BIN_DIR}/opencv_imgcodecs*.dll")
  file(GLOB _cv_vio    "${OPENCV_BIN_DIR}/opencv_videoio*.dll")

  set(_dlls ${_cv_ffmpeg})
  if (_cv_world)
    list(APPEND _dlls ${_cv_world})
  else()
    list(APPEND _dlls ${_cv_core} ${_cv_img} ${_cv_vio})
  endif()

  add_custom_command(TARGET simul_multi_file POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_dlls} $<TARGET_FILE_DIR:simul_multi_file>
    VERBATIM)
endif()

# ---- Copia config.txt accanto all'exe (se esiste nella root) ----
if (EXISTS "${CMAKE_SOURCE_DIR}/config.txt")
  add_custom_command(TARGET simul_multi_file POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/config.txt"
            "$<TARGET_FILE_DIR:simul_multi_file>/config.txt"
    VERBATIM)
endif()

# ---- Install: cartella dist pronta ----
install(TARGETS simul_multi_file RUNTIME DESTINATION .)
if (WIN32 AND OPENCV_BIN_DIR)
  install(FILES ${_dlls} DESTINATION .)
endif()
if (EXISTS "${CMAKE_SOURCE_DIR}/config.txt")
  install(FILES "${CMAKE_SOURCE_DIR}/config.txt" DESTINATION .)
endif()
