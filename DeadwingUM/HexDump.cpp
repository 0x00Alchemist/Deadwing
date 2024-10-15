#include <Windows.h>
#include <iostream>


namespace Hex {
	/**
	* \brief Dumps data of the provided address in HEX
	*
	* \param DataAddress Address of buffer with data
	* \param Length      Length of the buffer
	*/
	void
	WINAPI
	HexDump(
		_In_ PVOID  DataAddress,
		_In_ size_t Length
	) {
		UCHAR SymbolsSet[17] = { 0 };

		SymbolsSet[16] = '\0';
		for (int i = 0; i < Length; i++) {
			std::printf("%02X ", (reinterpret_cast<PUCHAR>(DataAddress))[i]);

			if ((reinterpret_cast<PUCHAR>(DataAddress))[i] > ' ' && (reinterpret_cast<PUCHAR>(DataAddress))[i] <= '~')
				SymbolsSet[i % 16] = ((PUCHAR)DataAddress)[i];
			else
				SymbolsSet[i % 16] = '.';

			if (((i + 1) % 8 == 0) || (i + 1 == Length)) {
				std::printf(" ");

				if (((i + 1) % 16) == 0) {
					std::printf("| %s\n", SymbolsSet);
				}
				else if (i + 1 == Length) {
					SymbolsSet[(i + 1) % 16] = '\0';

					if (((i + 1) % 16) <= 8)
						std::printf(" ");

					for (int j = ((i + 1) % 16); j < 16; j++)
						std::printf("\t");

					std::printf("| %s\n", SymbolsSet);
				}
			}
		}
	}
}

