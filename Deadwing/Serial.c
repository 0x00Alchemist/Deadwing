#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>
#include <Library/IoLib.h>

#include "Conf.h"

#define SERIAL_PORT_0  0x3F8

#ifdef DEADWING_QEMU_FIRMWARE 
#define OVMF_DEBUG_PORT 0x402
#endif

/// \note @0x00Alchemist: UART offsets
#define BAUD_LOW_OFFSET         0x00
#define BAUD_HIGH_OFFSET        0x01
#define IER_OFFSET              0x01
#define LCR_SHADOW_OFFSET       0x01
#define FCR_SHADOW_OFFSET       0x02
#define IR_CONTROL_OFFSET       0x02
#define FCR_OFFSET              0x02
#define EIR_OFFSET              0x02
#define BSR_OFFSET              0x03
#define LCR_OFFSET              0x03
#define MCR_OFFSET              0x04
#define LSR_OFFSET              0x05
#define MSR_OFFSET              0x06

/// \note @0x00Alchemist: UART registers
#define LSR_TXRDY               0x20
#define LSR_RXDA                0x01
#define DLAB                    0x01

/// \note @0x00Alchemist: Serial max baudrate
#define SERIAL_BAUDRATE_MAX 115200

/// \note @0x00ALchemist: UART settings
#define UART_DATA         8
#define UART_STOP         1
#define UART_PARITY       0
#define UART_BREAK_SET    0


/**
 * \brief Initializes UART port for data output
 * 
 * \param Port     Port to initialize
 * \param Baudrate Baudrate for port which will be initialized, baudrate should be less or equal SERIAL_BAUDRATE_MAX
 */
VOID
EFIAPI
SerialInitialize(
	IN UINT16 Port,
	IN UINT64 Baudrate
) {
	UINT16 DPort;

	UINT8 Data;
	UINT64 BaudrateDivisor;
	UINT64 OutputData;

	// save port value
	DPort = Port + LCR_OFFSET;

	// map data
	Data = (UINT8)(UART_DATA - 5);

	/// configure port communication format
	OutputData = (UINT8)((DLAB << 7) | (UART_BREAK_SET << 6) | (UART_PARITY << 3) | (UART_STOP << 2) | Data);
	IoWrite8(Port, OutputData);

	// calculate baudrate divisor for baud generator
	BaudrateDivisor = (UINT64)(SERIAL_BAUDRATE_MAX / Baudrate);

	// configure baudrate for serial port
	IoWrite8((Port + BAUD_HIGH_OFFSET), (UINT8)(BaudrateDivisor >> 8));
	IoWrite8((Port + BAUD_LOW_OFFSET), (UINT8)(BaudrateDivisor & 0xFF));

	// switch back. Port should be initialized for data output now
	OutputData = (UINT8)((~DLAB << 7) | (UART_BREAK_SET << 6) | (UART_PARITY << 3) | (UART_STOP << 2) | Data);
	IoWrite8(DPort, OutputData);
}

/**
 * \brief Writes data to port
 * 
 * \param Port  Port
 * \param Value Value to write
 */
VOID
EFIAPI
SerialWrite(
	IN UINT16 Port,
	IN UINT8  Value
) {
	UINT8 Status;
	UINT16 DPort;

	DPort = Port + LSR_OFFSET;

	// wait until port will be free
	Status = 0;
	do {
		Status = IoRead8(DPort);
	} while(Status & LSR_TXRDY == 0);

	// write data
	IoWrite8(Port, Value);
}

/**
 * \brief Reads data from given port
 * 
 * \param Port Port
 * 
 * \return Data from given port
 */
UINT8
EFIAPI
SerialRead(
	IN UINT16 Port
) {
	UINT8 Status;
	UINT16 DPort;

	DPort = Port + LSR_OFFSET;

	// wait until port will be free
	Status = 0;
	do {
		Status = IoRead8(DPort);
	} while(Status & LSR_RXDA == 0);

	return IoRead8(Port);
}

/**
 * \brief Prints message to port
 * 
 * \param Message Message to print
 */
VOID
EFIAPI
SerialPrint(
	IN CONST CHAR8 *Message
) {
	if(Message == NULL)
		return;

#ifndef DEADWING_QEMU_FIRMWARE
	// initialize serial port
	SerialInitialize(SERIAL_PORT_0, SERIAL_BAUDRATE_MAX);

	// send data to serial port
	for(INT32 i = 0; i < AsciiStrLen(Message); i++)
		SerialWrite(SERIAL_PORT_0, Message[i]);
#else
	// send data to QEMU debug port
	for(INT32 i = 0; i < AsciiStrLen(Message); i++)
		IoWrite8(OVMF_DEBUG_PORT, Message[i]);
#endif
}
