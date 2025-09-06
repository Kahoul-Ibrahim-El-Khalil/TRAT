#include "rat/system.hpp"

#ifdef _WIN32
#include <processthreadsapi.h>
#include <securitybaseapi.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace rat::system {

/*
 * Checks if the current process has elevated privileges.
 * - Windows: Administrator rights
 * - Linux/macOS: UID == 0
 */
bool isElevatedPrivileges(void) {
#ifdef _WIN32
	BOOL isElevated = FALSE;
	HANDLE token = nullptr;

		if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
			TOKEN_ELEVATION elevation;
			DWORD size = sizeof(elevation);

				if(GetTokenInformation(token, TokenElevation, &elevation,
				                       sizeof(elevation), &size)) {
					isElevated = elevation.TokenIsElevated;
				}
			CloseHandle(token);
		}
	return isElevated != FALSE;
#else
	return geteuid() == 0;
#endif
}

} // namespace rat::system
