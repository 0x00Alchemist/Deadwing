# DeadwingCppLib

**DeadwingCppLib** is a **static library** to facilitate and accelerate the development of usermode applications that need to access the **SMM driver through the kernel driver**. The list of available library functions can be seen below.

|   Command   |                                  Description                                    |
|    :---:    |                                     :---:                                       |
| `ping`      | Pings SMI handler                                                               |
| `cache`     | Caches information about current session (inlined into initialization function) |
| `physread`  | Reads from physical address                                                     |
| `varead`    | Reads from virtual address                                                      |
| `physwrite` | Writes to the physical address                                                  |
| `vawrite`   | Writes to the virtual address                                                   |
| `vtop`      | Translates virtual address to the physical address                              |
| `priv`      | Changes process token and spawns system shell                                   |

## Usage

```cpp
Deadwing::Commands DwCommands;

// initialize session
if(!DwCommands.InitializeSession(L"\\\\.\\DeadwingKM")) {
	std::wprintf(L"Cannot initialize session\n");
	return 1;
}

// ping SMM driver
if(!DwCommands.Ping()) {
	std::wprintf(L"SMM driver is unavailable\n");
	return 1;
}
```
