// SPDX-License-Identifier: BSD-3-Clause
/* types.hh - Common type definitions used in sycophant */
#pragma once
#if !defined(SYCOPHANT_TYPES_HH)
#define SYCOPHANT_TYPES_HH

#include <sys/mman.h>
#include <cstdint>

using void_t = void(*)();
using main_t = std::int32_t(*)(std::int32_t,  char**, char**);
using init_t = main_t;
using libc_start_main_t = std::int32_t(*)(main_t, std::int32_t, char**, void_t, void_t, void_t, void_t);
using dlsym_t = void*(*)(void*, const char*);

enum struct page_prot_t : std::int32_t {
	RWX = PROT_READ | PROT_WRITE | PROT_EXEC,
	RW  = PROT_READ | PROT_WRITE,
	RX  = PROT_READ | PROT_EXEC,
};


#endif /* SYCOPHANT_TYPES_HH */
