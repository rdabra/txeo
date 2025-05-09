
set(TXEO_SOURCES 
    utils.cpp 
    TensorShape.cpp 
    Tensor.cpp 
    Matrix.cpp
    Vector.cpp
    TensorOp.cpp 
    TensorAgg.cpp 
    TensorIO.cpp
    MatrixIO.cpp
    TensorPart.cpp 
    TensorFunc.cpp
    Predictor.cpp
    Trainer.cpp
    OlsGDTrainer.cpp
    Loss.cpp
    DataTable.cpp
    DataTableNorm.cpp
    Logger.cpp
    LoggerConsole.cpp
    LoggerFile.cpp)


#################################################### Finding Tensorflow ####################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(TensorFlow REQUIRED COMPONENTS CXX SHARED)


#################################################### Static Library ####################################################

add_library(txeo_static STATIC ${TXEO_SOURCES})
target_precompile_headers(txeo_static PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/txeo/detail/pch.h>"
)
target_include_directories(txeo_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:/include>
)
target_link_libraries(txeo_static PUBLIC TensorFlow::TensorFlow)
set_target_properties(txeo_static PROPERTIES OUTPUT_NAME "txeo") 


#################################################### Dynamic Library ####################################################
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

#Avoiding incompatibility issues between Ninja and Intel compiler
if(CMAKE_GENERATOR STREQUAL "Ninja" AND $ENV{CXX} STREQUAL "icpx") 
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
    set(CMAKE_SKIP_RPATH FALSE)
endif()
  
add_library(txeo_shared SHARED ${TXEO_SOURCES})
target_precompile_headers(txeo_shared PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/txeo/detail/pch.h>"
)
target_compile_definitions(txeo_shared PUBLIC TXEO_SHARED)
target_include_directories(txeo_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(txeo_shared PUBLIC TensorFlow::TensorFlow)
set_target_properties(txeo_shared PROPERTIES OUTPUT_NAME "txeo")

