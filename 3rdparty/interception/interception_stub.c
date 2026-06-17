/*
 * interception_stub.c - Stub DLL for the Interception keyboard/mouse filter library
 *
 * When the Interception kernel driver is not installed, windows-platform.dll
 * still needs interception.dll to load. This stub exports all required symbols
 * and returns NULL/no-op values so the app starts gracefully without the driver.
 *
 * Build (MinGW-w64 64-bit):
 *   gcc -shared -m64 -o interception.dll interception_stub.c
 *
 * The resulting interception.dll should be placed in the application directory.
 * Install the real Interception driver via interception/install-interception.exe
 * for full keyboard/mouse interception support on controlled machines.
 */

#include <windows.h>

typedef void* InterceptionContext;
typedef int InterceptionDevice;
typedef int InterceptionPrecedence;
typedef unsigned short InterceptionFilter;
typedef struct { char data[8]; } InterceptionStroke;
typedef int (*InterceptionPredicate)(InterceptionDevice);

__declspec(dllexport) InterceptionContext interception_create_context(void) { return NULL; }
__declspec(dllexport) void interception_destroy_context(InterceptionContext c) { (void)c; }
__declspec(dllexport) InterceptionPrecedence interception_get_precedence(InterceptionContext c, InterceptionDevice d) { (void)c; (void)d; return 0; }
__declspec(dllexport) void interception_set_precedence(InterceptionContext c, InterceptionDevice d, InterceptionPrecedence p) { (void)c; (void)d; (void)p; }
__declspec(dllexport) InterceptionFilter interception_get_filter(InterceptionContext c, InterceptionDevice d) { (void)c; (void)d; return 0; }
__declspec(dllexport) void interception_set_filter(InterceptionContext c, InterceptionPredicate pred, InterceptionFilter f) { (void)c; (void)pred; (void)f; }
__declspec(dllexport) InterceptionDevice interception_wait(InterceptionContext c) { (void)c; return -1; }
__declspec(dllexport) InterceptionDevice interception_wait_with_timeout(InterceptionContext c, unsigned long ms) { (void)c; (void)ms; return -1; }
__declspec(dllexport) int interception_send(InterceptionContext c, InterceptionDevice d, const InterceptionStroke *s, unsigned int n) { (void)c; (void)d; (void)s; (void)n; return 0; }
__declspec(dllexport) int interception_receive(InterceptionContext c, InterceptionDevice d, InterceptionStroke *s, unsigned int n) { (void)c; (void)d; (void)s; (void)n; return 0; }
__declspec(dllexport) unsigned int interception_get_hardware_id(InterceptionContext c, InterceptionDevice d, void *buf, unsigned int size) { (void)c; (void)d; (void)buf; (void)size; return 0; }
__declspec(dllexport) int interception_is_invalid(InterceptionDevice d) { return d <= 0; }
__declspec(dllexport) int interception_is_keyboard(InterceptionDevice d) { (void)d; return 0; }
__declspec(dllexport) int interception_is_mouse(InterceptionDevice d) { (void)d; return 0; }

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)hinstDLL; (void)fdwReason; (void)lpvReserved;
	return TRUE;
}
