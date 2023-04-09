# Define source files.
set(GROUNDLIFT_LIB_SOURCES
	groundlift/tcp.c
	groundlift/obex.c
	groundlift/server.c
	groundlift/client.c
	groundlift/utf16utils.c
	groundlift/error.c)

# Build the main groundlift library.
add_library(groundlift
			STATIC
			${GROUNDLIFT_LIB_SOURCES})

# Ensure everyone linking against us include our directory.
target_include_directories(groundlift
						   INTERFACE
						   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
						   $<INSTALL_INTERFACE:include>)
