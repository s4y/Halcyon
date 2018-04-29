SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x23000, LENGTH = 0x5d000
  RAM (rwx) :  ORIGIN = 0x20001368, LENGTH = 0xec98
  
}

SECTIONS
{
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
} INSERT AFTER .data;


INCLUDE "armgcc_s132_nrf52832_xxaa.ld"
