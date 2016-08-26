LIST(APPEND CONFIG_FILES
     header_html
     end_html
     theme.css
     script.js
    )

#always copy config files into build directory, so egui_cmd will be able to generate egui_auto
foreach (f ${CONFIG_FILES})
   configure_file(${f} ${CMAKE_CURRENT_BINARY_DIR}/${f} COPYONLY)
endforeach(f)
