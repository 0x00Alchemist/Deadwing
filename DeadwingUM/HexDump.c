#include <Windows.h>
#include <stdio.h>

/**
 * \brief Dumps data of the provided address in HEX
 * 
 * \param DataAddress Address of buffer with data
 * \param Length      Length of the buffer
 */
VOID
WINAPI
HexDump(
	_In_ VOID   *DataAddress,
	_In_ SIZE_T  Length
) {
	UCHAR SymbolsSet[17] = { 0 };

	SymbolsSet[16] = '\0';
	for(INT i = 0; i < Length; i++) {
		printf("%02X ", ((PUCHAR)DataAddress)[i]);
		
		if(((PUCHAR)DataAddress)[i] > ' ' && ((PUCHAR)DataAddress)[i] <= '~')
			SymbolsSet[i % 16] = ((PUCHAR)DataAddress)[i];
		else
			SymbolsSet[i % 16] = '.';
	
		if(((i + 1) % 8 == 0) || (i + 1 == Length)) {
			printf(" ");

			if(((i + 1) % 16) == 0) {
				printf("| %s\n", SymbolsSet);
			} else if (i + 1 == Length) {
				SymbolsSet[(i + 1) % 16] = '\0';
				
				if(((i + 1) % 16) <= 8)
					printf(" ");
			
				for(INT j = ((i + 1) % 16); j < 16; j++)
					printf("\t");

				printf("| %s\n", SymbolsSet);
			}
		}
	}
}
