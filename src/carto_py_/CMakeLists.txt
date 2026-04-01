message("-- loading submodule")

nanobind_add_module(
    carto_py_
    main.cpp
)

nanobind_add_stub(
    carto_py__stub
    MODULE carto_py_
    OUTPUT carto_py_.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:carto_py_>
    DEPENDS carto_py_
    MARKER_FILE py.typed
)

target_link_libraries(carto_py_ PUBLIC cartographer)
target_link_libraries(carto_py_ PUBLIC absl::strings absl::time)
target_link_libraries(carto_py_ PUBLIC boost_iostreams)

install(TARGETS carto_py_ LIBRARY DESTINATION ${CMAKE_PROJECT_NAME})

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/py.typed
    ${CMAKE_CURRENT_BINARY_DIR}/carto_py_.pyi
    DESTINATION ${CMAKE_PROJECT_NAME}
)
