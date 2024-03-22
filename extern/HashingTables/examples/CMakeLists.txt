add_executable(minimal
        minimal.cpp)

target_link_libraries(minimal PUBLIC
        HashingTables)

target_include_directories(minimal PRIVATE
        ${HASHING_TABLES_DIRECTORY})