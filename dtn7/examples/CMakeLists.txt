# Examples for the dtn7 module

# DTN7 basic example
build_lib_example(
  NAME dtn7-example
  SOURCE_FILES dtn7-example.cc
  LIBRARIES_TO_LINK 
    ${libdtn7}
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libapplications}
    ${libmobility}
    ${libnetanim}
)

# Copy examples to the build directory so that the config file is found
file(COPY 
     ${CMAKE_CURRENT_SOURCE_DIR}/dtn7-example.cc
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)