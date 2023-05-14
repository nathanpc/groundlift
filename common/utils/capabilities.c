/**
 * capabilities.c
 * Helper functions to determine the capabilities of a system at runtime.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "capabilities.h"

#ifdef _WIN32
#include <sysinfoapi.h>
#endif /* _WIN32 */

#ifdef _WIN32
/* Windows-specific capabilities. */
static OSVERSIONINFO m_osvi;

/**
 * Gets information about the current Windows version.
 * 
 * @return Windows version information structure.
 */
LPOSVERSIONINFO cap_win_ver(void) {
#pragma warning(push)
	/* Check if the version structure has already been populated. */
	if (m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO))
		return &m_osvi;

#pragma warning(disable : 4996)
	/* Setup the structure and get the version information. */
	ZeroMemory(&m_osvi, sizeof(OSVERSIONINFO));
	m_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&m_osvi);
#pragma warning(pop)

	return &m_osvi;
}

/**
 * Are we running on a computer with Windows XP or greater?
 * 
 * @return TRUE if the running under Windows XP or a later edition.
 */
bool cap_win_least_xp(void) {
	LPOSVERSIONINFO osvi = cap_win_ver();

	return (osvi->dwMajorVersion > 5) ||
		((osvi->dwMajorVersion == 5) && (osvi->dwMinorVersion >= 1));
}
#endif /* _WIN32 */

/**
 * Is the system capable of using, or at least converting, UTF-8 strings?
 * 
 * @return TRUE if the system can handle in some form UTF-8 strings.
 */
bool cap_utf8(void) {
#ifdef _WIN32
	return cap_win_least_xp();
#else
	return true;
#endif /* _WIN32 */
}
