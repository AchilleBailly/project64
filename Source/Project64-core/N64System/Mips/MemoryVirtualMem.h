#pragma once
#include <Project64-core\N64System\Mips\MemoryVirtualMem.h>
#include "TranslateVaddr.h"
#include <Project64-core\N64System\Recompiler\RecompilerOps.h>
#include <Project64-core\N64System\Interpreter\InterpreterOps.h>
#include <Project64-core\N64System\Mips\PifRam.h>
#include <Project64-core\N64System\Mips\FlashRam.h>
#include <Project64-core\N64System\Mips\Sram.h>
#include <Project64-core\N64System\Mips\Dma.h>
#include <Project64-core\N64System\MemoryHandler\CartridgeDomain2Address1Handler.h>
#include <Project64-core\N64System\MemoryHandler\DisplayControlRegHandler.h>
#include <Project64-core\N64System\MemoryHandler\MIPSInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\PeripheralInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMInterfaceHandler.h>
#include <Project64-core\N64System\MemoryHandler\RDRAMRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\SPRegistersHandler.h>
#include <Project64-core\N64System\MemoryHandler\VideoInterfaceHandler.h>
#include <Project64-core\Settings\GameSettings.h>

#ifdef __arm__
#include <sys/ucontext.h>
#endif

#ifndef _WIN32
#include <signal.h>
// siginfo_t
#endif

/*
* TODO:  Have address translation functions here?
* `return` either the translated address or the mask to XOR by?
*
* This will help us gradually be able to port Project64 to big-endian CPUs.
* Currently it is written to assume 32-bit little-endian, like so:
*
* 0xAABBCCDD EEFFGGHH --> 0xDDCCBBAA HHGGFFEE
*   GPR bits[63..0]         b1b2b3b4 b5b6b7b8
*/

#if defined(__i386__) || defined(_M_IX86)
class CX86RecompilerOps;
#elif defined(__arm__) || defined(_M_ARM)
class CArmRecompilerOps;
#endif

class CMipsMemoryVM :
    public CTransVaddr,
    private R4300iOp,
    public CPifRam,
    private CFlashram,
    private CSram,
    public CDMA,
    private CGameSettings
{
public:
    CMipsMemoryVM(CN64System & System, bool SavesReadOnly);
    ~CMipsMemoryVM();

    static void ReserveMemory();
    static void FreeReservedMemory();

    bool Initialize(bool SyncSystem);
    void Reset(bool EraseMemory);

    uint8_t * Rdram();
    uint32_t RdramSize();
    uint8_t * Dmem();
    uint8_t * Imem();
    uint8_t * PifRam();

    CSram * GetSram();
    CFlashram * GetFlashram();

    bool LB_VAddr(uint32_t VAddr, uint8_t & Value);
    bool LH_VAddr(uint32_t VAddr, uint16_t & Value);
    bool LW_VAddr(uint32_t VAddr, uint32_t & Value);
    bool LD_VAddr(uint32_t VAddr, uint64_t & Value);

    bool LB_PAddr(uint32_t PAddr, uint8_t & Value);
    bool LH_PAddr(uint32_t PAddr, uint16_t & Value);
    bool LW_PAddr(uint32_t PAddr, uint32_t & Value);
    bool LD_PAddr(uint32_t PAddr, uint64_t & Value);

    bool SB_VAddr(uint32_t VAddr, uint8_t Value);
    bool SH_VAddr(uint32_t VAddr, uint16_t Value);
    bool SW_VAddr(uint32_t VAddr, uint32_t Value);
    bool SD_VAddr(uint32_t VAddr, uint64_t Value);

    bool SB_PAddr(uint32_t PAddr, uint8_t Value);
    bool SH_PAddr(uint32_t PAddr, uint16_t Value);
    bool SW_PAddr(uint32_t PAddr, uint32_t Value);
    bool SD_PAddr(uint32_t PAddr, uint64_t Value);

    int32_t   MemoryFilter(uint32_t dwExptCode, void * lpExceptionPointer);

#ifndef _WIN32
    static bool SetupSegvHandler(void);
    static void segv_handler(int signal, siginfo_t *siginfo, void *sigcontext);
#endif

    // Protect the memory from being written to
    void ProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);
    void UnProtectMemory(uint32_t StartVaddr, uint32_t EndVaddr);

    // Functions for TLB notification
    void TLB_Mapped(uint32_t VAddr, uint32_t Len, uint32_t PAddr, bool bReadOnly);
    void TLB_Unmaped(uint32_t Vaddr, uint32_t Len);

    // CTransVaddr interface
    bool TranslateVaddr(uint32_t VAddr, uint32_t &PAddr) const;
    bool ValidVaddr(uint32_t VAddr) const;
    bool VAddrToRealAddr(uint32_t VAddr, void * &RealAddress) const;

    // Labels
    const char * LabelName(uint32_t Address) const;

    AudioInterfaceHandler & AudioInterface(void) { return m_AudioInterfaceHandler; }
    VideoInterfaceHandler & VideoInterface(void) { return m_VideoInterfaceHandler; }

private:
    CMipsMemoryVM();
    CMipsMemoryVM(const CMipsMemoryVM&);
    CMipsMemoryVM& operator=(const CMipsMemoryVM&);

#if defined(__i386__) || defined(_M_IX86)
    friend class CX86RecompilerOps;
#elif defined(__arm__) || defined(_M_ARM)
    friend class CArmRegInfo;
    friend class CArmRecompilerOps;
#endif

    static void RdramChanged(CMipsMemoryVM * _this);
    static void ChangeSpStatus();
    static void ChangeMiIntrMask();

    bool LB_NonMemory(uint32_t PAddr, uint32_t * Value, bool SignExtend);
    bool LH_NonMemory(uint32_t PAddr, uint32_t * Value, bool SignExtend);
    bool LW_NonMemory(uint32_t PAddr, uint32_t * Value);

    bool SB_NonMemory(uint32_t PAddr, uint8_t Value);
    bool SH_NonMemory(uint32_t PAddr, uint16_t Value);
    bool SW_NonMemory(uint32_t PAddr, uint32_t Value);

    static void Load32CartridgeDomain1Address1(void);
    static void Load32CartridgeDomain1Address3(void);
    static void Load32CartridgeDomain2Address2(void);
    static void Load32PifRam(void);
    static void Load32Rom(void);

    static void Write32CartridgeDomain2Address2(void);
    static void Write32PifRam(void);

#if defined(__i386__) || defined(_M_IX86)

    typedef struct _X86_CONTEXT
    {
        uint32_t * Edi;
        uint32_t * Esi;
        uint32_t * Ebx;
        uint32_t * Edx;
        uint32_t * Ecx;
        uint32_t * Eax;
        uint32_t * Eip;
        uint32_t * Esp;
        uint32_t * Ebp;
    } X86_CONTEXT;

    static bool FilterX86Exception(uint32_t MemAddress, X86_CONTEXT & context);
#endif
#ifdef __arm__
    static void DumpArmExceptionInfo(uint32_t MemAddress, mcontext_t & context);
    static bool FilterArmException(uint32_t MemAddress, mcontext_t & context);
#endif
    void FreeMemory();

    static uint8_t   * m_Reserve1, *m_Reserve2;
    CRegisters & m_Reg;
    AudioInterfaceHandler m_AudioInterfaceHandler;
    CartridgeDomain2Address1Handler m_CartridgeDomain2Address1Handler;
    DisplayControlRegHandler m_DPCommandRegistersHandler;
    MIPSInterfaceHandler m_MIPSInterfaceHandler;
    PeripheralInterfaceHandler m_PeripheralInterfaceHandler;
    RDRAMInterfaceHandler m_RDRAMInterfaceHandler;
    RDRAMRegistersHandler m_RDRAMRegistersHandler;
    SerialInterfaceHandler m_SerialInterfaceHandler;
    SPRegistersHandler m_SPRegistersHandler;
    VideoInterfaceHandler m_VideoInterfaceHandler;
    uint8_t * m_RDRAM, *m_DMEM, *m_IMEM;
    uint32_t m_AllocatedRdramSize;
    bool m_RomMapped;
    uint8_t * m_Rom;
    uint32_t m_RomSize;
    bool m_RomWrittenTo;
    uint32_t m_RomWroteValue;
    bool m_DDRomMapped;
    uint8_t * m_DDRom;
    uint32_t m_DDRomSize;

    mutable char m_strLabelName[100];
    size_t * m_TLB_ReadMap;
    size_t * m_TLB_WriteMap;

    static uint32_t m_MemLookupAddress;
    static MIPS_DWORD m_MemLookupValue;
    static bool m_MemLookupValid;
    static uint32_t RegModValue;
};
