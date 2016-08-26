set(CMAKE_BUILD_TYPE Debug)

set(PACKAGE_DATA_DIR_DEFINITION PACKAGE_DATA_DIR="${CMAKE_BINARY_DIR}/data/")

LIST(APPEND SOURCES accounting.c history_parser.c desc_parser.c common.c ui.c pdf_generator.c html_generator.c)

link_directories (${PC_LIBELEMENTARY_LIBRARY_DIRS})

include_directories(
   ${ELEMENTARY_INCLUDE_DIRS}
   ${EINA_INCLUDE_DIRS}
   ${EET_INCLUDE_DIRS}
   ${EO_INCLUDE_DIRS}
   ${ECORE_INCLUDE_DIRS}
   ${EVAS_INCLUDE_DIRS}
   )

add_executable(accounting ${SOURCES})

target_link_libraries(accounting
   pthread
   ${EFL_LIBRARIES}
   ${ELEMENTARY_LIBRARIES}
   ${EINA_LIBRARIES}
   ${EET_LIBRARIES}
   ${EO_LIBRARIES}
   ${ECORE_LIBRARIES}
   ${EVAS_LIBRARIES}
   ecore_file
   )

set_property(TARGET accounting
   PROPERTY COMPILE_DEFINITIONS ${PACKAGE_DATA_DIR_DEFINITION} 
)

install(TARGETS accounting DESTINATION bin)
