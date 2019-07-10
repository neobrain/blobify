add_executable(blobify-test simple_test)
target_link_libraries(blobify-test blobify)
add_test(blobify-test blobify-test)
