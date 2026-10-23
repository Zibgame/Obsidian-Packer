#include <stdio.h>
#include <windows.h>

static __thread int	tls_counter = 7;

static void NTAPI	tls_callback(void *handle, DWORD reason, void *reserved)
{
	(void)handle;
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH)
		printf("[tls_callback] process attach, image at %p\n", handle);
}

#pragma GCC diagnostic ignored "-Wattributes"
static PIMAGE_TLS_CALLBACK callback_ptr
	__attribute__((section(".CRT$XLB"), used)) = tls_callback;
int	main(void)
{
	printf("[tls_demo] tls_counter = %d\n", tls_counter);
	tls_counter++;
	printf("[tls_demo] tls_counter = %d\n", tls_counter);
	return (0);
}
