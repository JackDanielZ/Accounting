set(CMAKE_BUILD_TYPE Debug)

set(PACKAGE_DATA_DIR_DEFINITION PACKAGE_DATA_DIR="${CMAKE_BINARY_DIR}/data/")

LIST(APPEND SOURCES accounting.c history_parser.c desc_parser.c common.c html_generator.c list.c)

link_directories (${PC_LIBELEMENTARY_LIBRARY_DIRS})

add_executable(accounting ${SOURCES})

set_property(TARGET accounting
   PROPERTY COMPILE_DEFINITIONS ${PACKAGE_DATA_DIR_DEFINITION} 
)

install(TARGETS accounting DESTINATION bin)
