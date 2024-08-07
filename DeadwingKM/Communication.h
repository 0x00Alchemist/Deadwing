#pragma once

NTSTATUS
NTAPI
CommUmCmdHandler(
	_In_  PIRP               Irp,
	_In_  PIO_STACK_LOCATION IoStack,
	_Out_ PUINT64            OutputValue
);
