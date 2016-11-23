#ifndef _KERNEL_H_
#define _KERNEL_H_

#define KBASE_PHYSICAL          0x200000ULL
#define KBASE_VIRTUAL           0x10000000000ULL

typedef struct _KGLOBAL
{
    QWORD           PhysicalBase;
    QWORD           VirtualBase;
    SIZE_T          KernelSize;

    WORD            Phase;
} KGLOBAL, *PKGLOBAL;

VOID
KeInitGlobal(
    VOID
);

#endif // !_KERNEL_H_
