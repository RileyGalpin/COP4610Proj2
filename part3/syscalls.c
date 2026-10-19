#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/export.h>

/* Function pointers the elevator module fills in when loaded */
int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

SYSCALL_DEFINE0(start_elevator)
{
if (STUB_start_elevator)
return STUB_start_elevator();
return 0;
}

SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type)
{
if (STUB_issue_request)
return STUB_issue_request(start_floor, destination_floor, type);
return 0;
}

SYSCALL_DEFINE0(stop_elevator)
{
if (STUB_stop_elevator)
return STUB_stop_elevator();
return 0;
}
