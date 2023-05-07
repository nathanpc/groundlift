# Define source files.
set(GROUNDLIFT_COMMON_SOURCES
	utils/utf16.c
	utils/filesystem.c)

# Build the groundlift commons library.
add_library(glcommon
			STATIC
			${GROUNDLIFT_COMMON_SOURCES})

# Ensure everyone linking against us include our directory.
target_include_directories(glcommon
						   INTERFACE
						   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
						   $<INSTALL_INTERFACE:include>)
