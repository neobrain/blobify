add_executable(blobify-example test.cpp)
target_link_libraries(blobify-example PRIVATE blobify)

add_executable(example-bmp bmp.cpp)
target_link_libraries(example-bmp PRIVATE blobify)
