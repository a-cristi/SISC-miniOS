segment .text
[BITS 32]

;;
;; import / export entries between .nasm and .c parts
;;
%ifidn __OUTPUT_FORMAT__, win32
extern _EntryPoint                  ; import C entry point from main.c
EntryPoint equ _EntryPoint          ; win32 builds from Visual C decorate C names using _
%else
extern EntryPoint                   ; import C entry point from main.c
%endif

global gMultiBootHeader         ; export multiboot structures to .c
global gMultiBootStruct


;;
;; we use hardcoded address space / map for our data structures, the multiboot header and the entry point
;; the plain binary image is loaded to 0x00200000 (2MB)
;;
KERNEL_BASE_VIRTUAL_32      equ 0x40000000			    ; magic 1G VA for x86 builds
KERNEL_BASE_VIRTUAL_64      equ 0x0000000200000000	    ; magic 8G VA for x64 builds
KERNEL_BASE_PHYSICAL        equ 0x200000                ; physical address where this file will be loaded (2 MB PA)

MULTIBOOT_HEADER_BASE       equ KERNEL_BASE_PHYSICAL + 0x400 ; take into account the MZ/PE header + 0x400 allignment
                                                        ; the multiboot header begins in the .text section
MULTIBOOT_HEADER_SIZE       equ 48                      ; check out '3.1.1 The layout of Multiboot header'
MULTIBOOT_HEADER_MAGIC      equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS      equ 0x00010003              ; 0x1 ==> loading of modules must pe 4K alligned, 0x2 ==> OS needs memory map
                                                        ; 0x10000 ==> OS image has valid header_addr, load_addr, ..., entry_addr

MULTIBOOT_INFO_STRUCT_BASE  equ MULTIBOOT_HEADER_BASE + MULTIBOOT_HEADER_SIZE
MULTIBOOT_INFO_STRUCT_SIZE  equ 90

MULTIBOOT_ENTRY_POINT       equ (gMultiBootEntryPoint - gMultiBootHeader) + KERNEL_BASE_PHYSICAL + 0x400

IA32_EFER                   equ 0xC0000080
CR4_PAE                     equ 0x00000020
IA23_EFER_LME               equ 0x100


TOP_OF_STACK_VIRTUAL        equ KERNEL_BASE_VIRTUAL_64 + 0x10000



;;
;; KERNEL_BASE_PHYSICAL + 0x400
;;
;; *** IMPORTANT: __init.nasm MUST be the first object to be linked into the code segment ***
;;

gMultiBootHeader:                                       ; check out '3.1.1 The layout of Multiboot header'
.magic          dd MULTIBOOT_HEADER_MAGIC
.flags          dd MULTIBOOT_HEADER_FLAGS
.checksum       dd 0-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
.header_addr    dd MULTIBOOT_HEADER_BASE
.load_addr      dd KERNEL_BASE_PHYSICAL
.load_end_addr  dd 0
.bss_end_addr   dd 0
.entry_addr     dd MULTIBOOT_ENTRY_POINT
.mode_type      dd 0
.width          dd 0
.height         dd 0
.depth          dd 0

gMultiBootStruct:                                       ; reserve space for the multiboot info structure (will copy here)
times MULTIBOOT_INFO_STRUCT_SIZE db 0                   ; check out '3.3 Boot information format'


;; leave 0x40 bytes for GDT stuff
times (0x100 - MULTIBOOT_HEADER_SIZE - MULTIBOOT_INFO_STRUCT_SIZE - 0x40) db 0

;;
;; N.B. here we have plenty of space (over 60KB to be used to define various data structures needed, e.g. page tables)
;;

;;
;; KERNEL_BASE_PHYSICAL + 0x4C0
;;


;;
;; TOP-OF-STACK is KERNEL_BASE_PHYSICAL + 0x10000
;;


;;
;; N.B. multiboot starts in 32 bit PROTECTED MODE, without paging beeing enabled (FLAT); check out '3.2 Machine state' from docs
;; we explicitly allign the entry point to +64 KB (0x10000)
;;

times 0x10000-0x400-$+gMultiBootHeader db 'G'           ; allignment


;;
;; KERNEL_BASE_PHYSICAL + 0x10000
;;
[BITS 32]
gMultiBootEntryPoint:
    cli

    MOV     DWORD [0x000B8000], 'O1S1'
%ifidn __OUTPUT_FORMAT__, win32
    MOV     DWORD [0x000B8004], '3121'                  ; 32 bit build marker
%else
    MOV     DWORD [0x000B8004], '6141'                  ; 64 bit build marker
%endif

    ;; enable SSE instructions (CR4.OSFXSR = 1)
    MOV     EAX, CR4
    OR      EAX, 0x00000200
    MOV     CR4, EAX

    MOV     ESP, KERNEL_BASE_PHYSICAL                   ; temporary ESP, just below code
    
    CALL EntryPoint
    
	MOV     DWORD [0x000B80FC], ' 0#0'
    MOV     DWORD [0x000B8100], 'S0E0'       
    MOV     DWORD [0x000B8104], 'C0U0'       
    MOV     DWORD [0x000B8108], 'R0E0'
    
    CLI
    HLT

;;--------------------------------------------------------
;; EXPORT TO C FUNCTIONS
;;--------------------------------------------------------

;%ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
;global ___cli
;___cli equ __cli
;%else
;global __cli
;%endif

%macro EXPORT2C 1-*
%rep  %0
    %ifidn __OUTPUT_FORMAT__, win32 ; win32 builds from Visual C decorate C names using _ 
    global _%1
    _%1 equ %1
    %else
    global %1
    %endif
%rotate 1 
%endrep
%endmacro

EXPORT2C __cli, __sti, __magic

__cli:
    CLI
    RET

__sti:
    STI
    RET

__magic:
    XCHG    BX,BX
    RET

