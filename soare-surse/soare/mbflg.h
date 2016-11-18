#ifndef _MBFLG_H_
#define _MBFLG_H_

// See https://www.gnu.org/software/grub/manual/multiboot/html_node/Boot-information-format.html#Boot-information-format

#define MBOOT_FLG_LU_MEM            BIT(0)  // 1 - mem_lower and mem_upper fields are valid
#define MBOOT_FLG_BOOT_DEV          BIT(1)  // 1 - boot_device field is valid
#define MBOOT_FLG_CMD_LINE          BIT(2)  // 1 - cmdline field is valid (0-terminated string)
#define MBOOT_FLG_MODS              BIT(3)  // 1 - mods_count = number of modules loaded, mods_adder = start of mods array
#define MBOOT_FLG_AOUT_VALID        BIT(4)  // 1 - a.out kernel image symbol related info; MBOOT_FLG_ELF_VALID must be 0
#define MBOOT_FLG_ELF_VALID         BIT(5)  // 1 - ELF kernel imAge section header table info; MBOOT_FLG_AOUT_VALID must be 0
#define MBOOT_FLG_MMAP              BIT(6)  // 1 - mmap_* fields are valid
#define MBOOT_FLG_DRIVES            BIT(7)  // 1 - the drives_* fields are valid
#define MBOOT_FLG_CFG_TABLE         BIT(8)  // 1 - config_table is valid
#define MBOOT_FLG_BOOT_LOADER       BIT(9)  // 1 - boot_loader_name is valid
#define MBOOT_FLG_APM               BIT(10) // 1 - apm_table is valid; see http://www.microsoft.com/hwdev/busbios/amp_12.htm
#define MBOOT_FLG_GRAPHICS_TABLE    BIT(11) // 1 - graphics table available (only if the kernel has indicated via the Multiboot header that it accepts graphics mode)

#endif // !_MBFLG_H_
