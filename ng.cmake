# ISLPlacer.dll on alandtse's CommonLibSSE-NG (Skyrim 1.5.97 to 1.6.1170 and 1.7.x, SE and AE); included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there).
ng_plugin(TARGET ISLPlacer NAME ISLPlacer VERSION 0.2.0 AUTHOR "CageTV" ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES src/main.cpp src/Menu.cpp src/Editor.cpp
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/src"
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h"
    LIBS nlohmann_json::nlohmann_json)
