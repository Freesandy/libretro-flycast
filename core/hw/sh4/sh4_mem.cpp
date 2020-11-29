/*
	Mostly buggy, old, glue code that somehow still works
	Most of the work is now delegated on vtlb and only helpers are here
*/
#include "types.h"

#include "sh4_mem.h"
#include "hw/holly/sb_mem.h"
#include "sh4_mmr.h"
#include "modules/modules.h"
#include "hw/pvr/pvr_mem.h"
#include "hw/sh4/sh4_core.h"
#include "hw/mem/_vmem.h"
#include "modules/mmu.h"
#include "sh4_cache.h"

//main system mem
VArray2 mem_b;

#ifndef NO_MMU
// Memory handlers
ReadMem8Func ReadMem8;
ReadMem16Func ReadMem16;
ReadMem16Func IReadMem16;
ReadMem32Func ReadMem32;
ReadMem64Func ReadMem64;

WriteMem8Func WriteMem8;
WriteMem16Func WriteMem16;
WriteMem32Func WriteMem32;
WriteMem64Func WriteMem64;
#endif

//MEM MAPPINNGG

//AREA 1
static _vmem_handler area1_32b;

static void map_area1_init()
{
	area1_32b = _vmem_register_handler(pvr_read_area1_8,pvr_read_area1_16,pvr_read_area1_32,
									pvr_write_area1_8,pvr_write_area1_16,pvr_write_area1_32);
}

static void map_area1(u32 base)
{
	//map vram
	
	//Lower 32 mb map
	//64b interface
	_vmem_map_block(vram.data,0x04 | base,0x04 | base,VRAM_SIZE-1);
	//32b interface
	_vmem_map_handler(area1_32b,0x05 | base,0x05 | base);
	
	//Upper 32 mb mirror
	//0x0600 to 0x07FF
	_vmem_mirror_mapping(0x06|base,0x04|base,0x02);
}

//AREA 2
static void map_area2_init()
{
	//nothing to map :p
}

static void map_area2(u32 base)
{
	//nothing to map :p
}


//AREA 3
static void map_area3_init()
{
}

static void map_area3(u32 base)
{
	//32x2 or 16x4
	_vmem_map_block_mirror(mem_b.data,0x0C | base,0x0F | base,RAM_SIZE);
}

//AREA 4
static void map_area4_init()
{
	
}

static void map_area4(u32 base)
{
	//TODO : map later

	//upper 32mb mirror lower 32 mb
	_vmem_mirror_mapping(0x12|base,0x10|base,0x02);
}


//AREA 5	--	Ext. Device
//Read Ext.Device
template <class T>
T DYNACALL ReadMem_extdev_T(u32 addr)
{
	return (T)libExtDevice_ReadMem_A5(addr, sizeof(T));
}

//Write Ext.Device
template <class T>
void DYNACALL WriteMem_extdev_T(u32 addr,T data)
{
	libExtDevice_WriteMem_A5(addr, data, sizeof(T));
}

_vmem_handler area5_handler;
static void map_area5_init()
{
	area5_handler = _vmem_register_handler_Template(ReadMem_extdev_T,WriteMem_extdev_T);
}

static void map_area5(u32 base)
{
	//map whole region to plugin handler :)
	_vmem_map_handler(area5_handler,base|0x14,base|0x17);
}

//AREA 6	--	Unassigned 
static void map_area6_init()
{
	//nothing to map :p
}
static void map_area6(u32 base)
{
	//nothing to map :p
}


//set vmem to default values
void mem_map_default()
{
	//vmem - init/reset :)
	_vmem_init();

	
	//*TEMP*
	//setup a fallback handler , that calls old code :)
	//_vmem_handler def_handler =
	//	_vmem_register_handler(ReadMem8_i,ReadMem16_i,ReadMem32_i,WriteMem8_i,WriteMem16_i,WriteMem32_i);
	//_vmem_map_handler(def_handler,0,0xFFFF);

	//U0/P0
	//0x0xxx xxxx	-> normal memmap
	//0x2xxx xxxx	-> normal memmap
	//0x4xxx xxxx	-> normal memmap
	//0x6xxx xxxx	-> normal memmap
	//-----------
	//P1
	//0x8xxx xxxx	-> normal memmap
	//-----------
	//P2
	//0xAxxx xxxx	-> normal memmap
	//-----------
	//P3
	//0xCxxx xxxx	-> normal memmap
	//-----------
	//P4
	//0xExxx xxxx	-> internal area

	//Init Memmaps (register handlers)
	map_area0_init();
	map_area1_init();
	map_area2_init();
	map_area3_init();
	map_area4_init();
	map_area5_init();
	map_area6_init();
	map_area7_init();

	//0x0-0xD : 7 times the normal memmap mirrors :)
	//some areas can be customised :)
	for (int i=0x0;i<0xE;i+=0x2)
	{
		map_area0(i<<4); //Bios,Flahsrom,i/f regs,Ext. Device,Sound Ram
		map_area1(i<<4); //VRAM
		map_area2(i<<4); //Unassigned
		map_area3(i<<4); //RAM
		map_area4(i<<4); //TA
		map_area5(i<<4); //Ext. Device
		map_area6(i<<4); //Unassigned
		map_area7(i<<4); //Sh4 Regs
	}

	//map p4 region :)
	map_p4();
}
void mem_Init()
{
	//Allocate mem for memory/bios/flash
	//mem_b.Init(&sh4_reserved_mem[0x0C000000],RAM_SIZE);

	sh4_area0_Init();
	sh4_mmr_init();
	MMU_init();
}

//Reset Sysmem/Regs -- Pvr is not changed , bios/flash are not zeroed out
void mem_Reset(bool hard)
{
	//mem is reseted on hard restart(power on) , not manual...
	if (hard)
	{
		//fill mem w/ 0's
		mem_b.Zero();
	}

	//Reset registers
	sh4_area0_Reset(hard);
	sh4_mmr_reset(hard);
	MMU_reset();
}

void mem_Term()
{
	MMU_term();
	sh4_mmr_term();
	sh4_area0_Term();

	_vmem_term();
}

void WriteMemBlock_nommu_dma(u32 dst,u32 src,u32 size)
{
	u32 dst_msk,src_msk;

	void* dst_ptr=_vmem_get_ptr2(dst,dst_msk);
	void* src_ptr=_vmem_get_ptr2(src,src_msk);

	if (dst_ptr && src_ptr)
	{
		memcpy((u8*)dst_ptr+(dst&dst_msk),(u8*)src_ptr+(src&src_msk),size);
	}
	else if (src_ptr)
	{
		WriteMemBlock_nommu_ptr(dst,(u32*)((u8*)src_ptr+(src&src_msk)),size);
	}
	else
	{
		verify(size % 4 == 0);
		for (u32 i=0;i<size;i+=4)
		{
			WriteMem32_nommu(dst+i,ReadMem32_nommu(src+i));
		}
	}
}
void WriteMemBlock_nommu_ptr(u32 dst,u32* src,u32 size)
{
	u32 dst_msk;

	void* dst_ptr=_vmem_get_ptr2(dst,dst_msk);

	if (dst_ptr)
	{
		dst&=dst_msk;
		memcpy((u8*)dst_ptr+dst,src,size);
	}
	else
	{
		for (u32 i = 0; i < size;)
		{
			u32 left = size - i;
			if (left >= 4)
			{
				WriteMem32_nommu(dst + i, src[i >> 2]);
				i += 4;
			}
			else if (left >= 2)
			{
				WriteMem16_nommu(dst + i, ((u16 *)src)[i >> 1]);
				i += 2;
			}
			else
			{
				WriteMem8_nommu(dst + i, ((u8 *)src)[i]);
				i++;
			}
		}
	}
}

void WriteMemBlock_nommu_sq(u32 dst,u32* src)
{
	u32 dst_msk;
	void* dst_ptr=_vmem_get_ptr2(dst,dst_msk);

	if (dst_ptr)
	{
		dst&=dst_msk;
		memcpy((u8*)dst_ptr+dst,src,32);
	}
	else
	{
		for (u32 i=0;i<32;i+=4)
		{
			WriteMem32_nommu(dst+i,src[i>>2]);
		}
	}
}

//Get pointer to ram area , 0 if error
//For debugger(gdb) - dynarec
u8* GetMemPtr(u32 Addr,u32 size)
{
	verify((((Addr>>29) &0x7)!=7));
	switch ((Addr>>26)&0x7)
	{
		case 3:
		return &mem_b[Addr & RAM_MASK];
		
		case 0:
		case 1:
		case 2:
		case 4:
		case 5:
		case 6:
		case 7:
		default:
//			INFO_LOG(COMMON, "unsupported area : addr=0x%X", Addr);
			return 0;
	}
}

bool IsOnRam(u32 addr)
{
	if (((addr>>26)&0x7)==3)
	{
		if ((((addr>>29) &0x7)!=7) && (((addr>>29) &0x7)!=3))
		{
			return true;
		}
	}

	return false;
}

static bool interpreterRunning = false;

void SetMemoryHandlers()
{
#ifndef NO_MMU
#ifdef STRICT_MODE
	if (settings.dynarec.Enable && interpreterRunning)
	{
		// Flush caches when interp -> dynarec
		ocache.WriteBackAll();
		icache.Invalidate();
	}

	if (!settings.dynarec.Enable)
	{
		interpreterRunning = true;
		IReadMem16 = &IReadCachedMem;
		ReadMem8 = &ReadCachedMem<u8>;
		ReadMem16 = &ReadCachedMem<u16>;
		ReadMem32 = &ReadCachedMem<u32>;
		ReadMem64 = &ReadCachedMem<u64>;

		WriteMem8 = &WriteCachedMem<u8>;
		WriteMem16 = &WriteCachedMem<u16>;
		WriteMem32 = &WriteCachedMem<u32>;
		WriteMem64 = &WriteCachedMem<u64>;

		return;
	}
	interpreterRunning = false;
#endif
	if (CCN_MMUCR.AT == 1 && settings.dreamcast.FullMMU)
	{
		IReadMem16 = &mmu_IReadMem16;
		ReadMem8 = &mmu_ReadMem<u8>;
		ReadMem16 = &mmu_ReadMem<u16>;
		ReadMem32 = &mmu_ReadMem<u32>;
		ReadMem64 = &mmu_ReadMem<u64>;

		WriteMem8 = &mmu_WriteMem<u8>;
		WriteMem16 = &mmu_WriteMem<u16>;
		WriteMem32 = &mmu_WriteMem<u32>;
		WriteMem64 = &mmu_WriteMem<u64>;
	}
	else
	{
		ReadMem8 = &_vmem_ReadMem8;
		ReadMem16 = &_vmem_ReadMem16;
		IReadMem16 = &_vmem_ReadMem16;
		ReadMem32 = &_vmem_ReadMem32;
		ReadMem64 = &_vmem_ReadMem64;

		WriteMem8 = &_vmem_WriteMem8;
		WriteMem16 = &_vmem_WriteMem16;
		WriteMem32 = &_vmem_WriteMem32;
		WriteMem64 = &_vmem_WriteMem64;
	}
#endif
}
