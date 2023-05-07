# Define source files.
set(GROUNDLIFT_COMMON_SOURCES
	utils/utf16.c
	utils/filesystem.c
	utils/threads.c)

# Build the groundlift commons library.
add_library(glcommon
			STATIC
			${GROUNDLIFT_COMMON_SOURCES})

# Ensure we have pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(glcommon PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(glcommon PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Ensure everyone linking against us include our directory.
target_include_directories(glcommon
						   INTERFACE
						   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
						   $<INSTALL_INTERFACE:include>)
