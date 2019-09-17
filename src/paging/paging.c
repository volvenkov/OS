#include "../libs/kmalloc.h"
#include "../libs/mem.h"
#include "../libs/shell.h"
#include "frame_alloc.h"
#include "paging.h"

#define PLACEMENT_ADDRESS 0x300000

void page_fault(registers_t regs) {
	shell_print("PAGE FAULT!");
	return;
}

void initialise_paging(uint32_t mem_end_page) {
	set_placement_addr(PLACEMENT_ADDRESS);

	uint32_t nframes = mem_end_page / 0x1000;
	uint32_t* frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes) * 4);
	init_frames_bit_set(nframes, frames);
	memory_set(frames, 0, INDEX_FROM_BIT(nframes) * 4);
	
	page_directory_t* kernel_dir = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
	memory_set(kernel_dir, 0, sizeof(page_directory_t));

	for (uint32_t i = 0; i < get_placement_addr(); i += 0x1000) {
		alloc_frame(get_page(i, 1, kernel_dir), 0, 0);
	}

    register_interrupt_handler(14, page_fault);

	switch_page_directory(kernel_dir);

	return;
}

void switch_page_directory(page_directory_t* dir) {
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0":: "r"(cr0));
	return;
}

page_t* get_page(uint32_t address, int make, page_directory_t* dir) {
    address /= 0x1000;
    uint32_t table_idx = address / 1024;
    if (dir->tables[table_idx]) {
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else if(make) {
        uint32_t tmp;
        dir->tables[table_idx] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &tmp);
        memory_set(dir->tables[table_idx], 0, 0x1000);
        dir->tablesPhysical[table_idx] = tmp | 0x7;
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else {
        return 0;
    }
	return 0;
}