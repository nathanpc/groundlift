# Define source files.
set(GROUNDLIFT_LIB_SOURCES
	groundlift/sockets.c
	groundlift/obex.c
	groundlift/server.c
	groundlift/client.c
	groundlift/conf.c
	groundlift/error.c)

# Build the main groundlift library.
add_library(groundlift
			STATIC
			${GROUNDLIFT_LIB_SOURCES})

# Ensure we have pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(groundlift PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(groundlift PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Ensure we have our common components in the include path.
target_link_libraries(groundlift PUBLIC glcommon)

# Ensure everyone linking against us include our directory.
target_include_directories(groundlift
						   INTERFACE
						   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
						   $<INSTALL_INTERFACE:include>)
