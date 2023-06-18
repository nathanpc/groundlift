# Define source files.
set(GROUNDLIFT_DBUS_SOURCES
	main.c
	gdbus.c
	client.c)

# Use a newer standard of the C language since this is Linux.
set(CMAKE_C_STANDARD 99)

# Build the D-bus service.
add_executable(gld
			   ${GROUNDLIFT_DBUS_SOURCES})

# Ensure we have pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(gl PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(gl PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Ensure we have GLib and GIO.
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-2.0)
pkg_search_module(GIOUNIX REQUIRED gio-unix-2.0)
target_include_directories(gld PRIVATE ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS}
	${GIOUNIX_INCLUDE_DIRS})
target_link_libraries(gld PUBLIC ${GLIB_LDFLAGS} ${GIO_LDFLAGS}
	${GIOUNIX_LDFLAGS})

# Ensure we have our common components and main library in the include path.
target_link_libraries(gld PUBLIC glcommon)

# Ensure we have our main library in the include path.
target_link_libraries(gld PUBLIC groundlift)
