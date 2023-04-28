#include <address_space.h>
#define RESULT_OK 0
#define ARRAY_LENGTH(a) sizeof((a))/sizeof((a)[0])

typedef enum {
	INVALID,
	RESERVED_BY_KERNEL,
	RESERVED_BY_USER,
} memory_region_state_t;

typedef struct {
	memory_region_state_t state;
	uint64_t base;
	size_t size;
} memory_region_t;

static Result as_set_region_from_info(memory_region_t *region, int base_id, int size_id, memory_region_state_t state) {
	Result r;

	if(region == NULL) {
		return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
	}
	
	region->state = state;
	if((r = svcGetInfo(&region->base, base_id, 0xFFFF8001, 0)) != RESULT_OK) { return r; }
	if((r = svcGetInfo(&region->size, size_id, 0xFFFF8001, 0)) != RESULT_OK) { return r; }

	return RESULT_OK;
}

static memory_region_t regions[256];
static uint64_t num_regions = 0;
static memory_region_t address_space;

static memory_region_t *as_grab_region() {
	if(num_regions < ARRAY_LENGTH(regions)) {
		return &regions[num_regions++];
	}
	return NULL;
}

static void as_delete_region(uint64_t index) {
	memmove(&regions[index], &regions[index+1], (--num_regions)-index);
}

Result as_init() {	
	for(uint64_t i = 0; i < ARRAY_LENGTH(regions); i++) {
		regions[i].state = INVALID;
	}
	
	Result r;
	
	if((r = as_set_region_from_info(as_grab_region(), 2, 3, RESERVED_BY_KERNEL)) != RESULT_OK) { // MapRegion
		return r;
	}
	
	if((r = as_set_region_from_info(as_grab_region(), 4, 5, RESERVED_BY_KERNEL)) != RESULT_OK) { // HeapRegion
		return r;
	}

	if(true) {
		if((r = as_set_region_from_info(as_grab_region(), 14, 15, RESERVED_BY_KERNEL)) != RESULT_OK) { // NewMapRegion
			return r;
		}
		
		if((r = as_set_region_from_info(&address_space, 12, 13, INVALID)) != RESULT_OK) { // AddressSpace
			return r;
		}
	} else {
		r = svcUnmapMemory((void*) 0xffffffffffffe000, (void *) ((2^36) - 0x2000), 0x1000);
		if(r == 0xdc01) { // invalid destination address
			// source 36-bit address was valid
			address_space.base = 0x8000000;
			address_space.size = (2^36) - address_space.base;
		} else {
			// let's just assume 32-bit
			address_space.base = 0x200000;
			address_space.size = (2^32) - address_space.base;
		}
	}
	return RESULT_OK;
}

void as_finalize() {
}

void *as_reserve(size_t len) {	
	uint64_t addr;
	MemoryInfo memory_info;
	Result r;
	uint32_t page_info;
	
	do {
		uint64_t random = (uint64_t) rand() << 12;
		addr = (random % address_space.size) + address_space.base;

		bool is_overlapping = false;
		for(uint64_t i = 0; i < num_regions; i++) {
			memory_region_t *r = &regions[i];
			if(r->state != RESERVED_BY_USER && r->state != RESERVED_BY_KERNEL) {
				continue;
			}
			
			if((addr >= r->base && addr < (r->base + r->size)) ||
			   ((addr + len) >= r->base && (addr + len) < (r->base + r->size)) ||
			   (r->base >= addr && r->base < (addr + len)) ||
			   ((r->base + r->size) >= addr && (r->base + r->size) < (addr + len))) {
				is_overlapping = true;
				break;
			}
		}

		if(is_overlapping) {
			continue;
		}
		
		if((r = svcQueryMemory(&memory_info, &page_info, (void*) addr)) != RESULT_OK) {
			goto fail_mutex;
		}
	} while(memory_info.type != 0 || memory_info.attr != 0 || memory_info.perm != 0 || (uint64_t) memory_info.addr + memory_info.size < addr + len);

	memory_region_t *region = as_grab_region();
	if(region == NULL) {
		goto fail_mutex;
	}

	region->state = RESERVED_BY_USER;
	region->base = addr;
	region->size = len;

	return (void*) addr;

fail_mutex:
	return NULL;
}

void as_release(void *addr, size_t len) {
	for(uint64_t i = 0; i < num_regions; i++) {
		if(regions[i].base == (uint64_t) addr) {
			if(regions[i].size != len || regions[i].state != RESERVED_BY_USER) {
				return;
			}
			as_delete_region(i);
			return;
		}
	}
}