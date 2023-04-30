# Define source files.
set(GROUNDLIFT_CLI_SOURCES
	main.c
	client.c
	server.c)

# Build the server/client command-line utility.
add_executable(gl
			   ${GROUNDLIFT_CLI_SOURCES})

# Ensure we have pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(gl PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(gl PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Ensure we have our main library in the include path.
target_link_libraries(gl PUBLIC groundlift)
