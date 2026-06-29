/*
 * Minimal Interception SDK header used by the Windows platform plugin.
 *
 * The CI build links against interception_stub.c. Install the real
 * Interception driver for full keyboard/mouse interception support.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void* InterceptionContext;
typedef int InterceptionDevice;
typedef int InterceptionPrecedence;
typedef unsigned short InterceptionFilter;
typedef struct { char data[8]; } InterceptionStroke;
typedef int (*InterceptionPredicate)(InterceptionDevice);

#define INTERCEPTION_FILTER_KEY_ALL 0xFFFF
#define INTERCEPTION_FILTER_MOUSE_ALL 0xFFFF

InterceptionContext interception_create_context(void);
void interception_destroy_context(InterceptionContext context);
InterceptionPrecedence interception_get_precedence(InterceptionContext context, InterceptionDevice device);
void interception_set_precedence(InterceptionContext context, InterceptionDevice device, InterceptionPrecedence precedence);
InterceptionFilter interception_get_filter(InterceptionContext context, InterceptionDevice device);
void interception_set_filter(InterceptionContext context, InterceptionPredicate predicate, InterceptionFilter filter);
InterceptionDevice interception_wait(InterceptionContext context);
InterceptionDevice interception_wait_with_timeout(InterceptionContext context, unsigned long milliseconds);
int interception_send(InterceptionContext context, InterceptionDevice device, const InterceptionStroke* stroke, unsigned int count);
int interception_receive(InterceptionContext context, InterceptionDevice device, InterceptionStroke* stroke, unsigned int count);
unsigned int interception_get_hardware_id(InterceptionContext context, InterceptionDevice device, void* buffer, unsigned int size);
int interception_is_invalid(InterceptionDevice device);
int interception_is_keyboard(InterceptionDevice device);
int interception_is_mouse(InterceptionDevice device);

#ifdef __cplusplus
}
#endif
