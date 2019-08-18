CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)
project (Lot49)

# The version number.
set (Lot49_VERSION_MAJOR 0)
set (Lot49_VERSION_MINOR 1)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_BUILD_TYPE "DEBUG")


include_directories(
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/.libs/include
  ${CMAKE_CURRENT_SOURCE_DIR}/zkp-hash
  )

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/.libs/lib
  )

find_library(secp256k1 NAMES libsecp256k1.a PATHS ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1-zkp/.libs/lib)

add_executable(lot49 lot49.cpp MeshNode.cpp ImpliedTransaction.cpp Utils.cpp)
target_link_libraries(lot49 secp256k1)
