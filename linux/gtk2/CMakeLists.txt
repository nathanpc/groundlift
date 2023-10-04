# Define source files.
set(GROUNDLIFT_GTK2_SOURCES
	main.c
	server.c
	comdlg.c
	windows/sendfile.c
	windows/transfer.c
	windows/recvprogress.c
	windows/sendprogress.c)

# Use a newer standard of the C language since this is Linux.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
add_compile_definitions(_GNU_SOURCE)

# Build the GTK2 application.
add_executable(gl-gtk2
			   ${GROUNDLIFT_GTK2_SOURCES})

# Ensure we have pthreads.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(gl-gtk2 PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(gl-gtk2 PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()

# Ensure we have GLib and GIO.
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GTK REQUIRED gtk+-2.0)
target_include_directories(gl-gtk2 PRIVATE ${GLIB_INCLUDE_DIRS} ${GTK_INCLUDE_DIRS})
target_link_libraries(gl-gtk2 PUBLIC ${GLIB_LDFLAGS} ${GTK_LDFLAGS})

# Ensure we have AppIndicators.
pkg_search_module(APPINDICATORS REQUIRED ayatana-appindicator-0.1)
target_include_directories(gl-gtk2 PRIVATE ${APPINDICATORS_INCLUDE_DIRS})
target_link_libraries(gl-gtk2 PUBLIC ${APPINDICATORS_LDFLAGS})

# Ensure we have our common components and main library in the include path.
target_link_libraries(gl-gtk2 PUBLIC glcommon)

# Ensure we have our main library in the include path.
target_link_libraries(gl-gtk2 PUBLIC groundlift)
