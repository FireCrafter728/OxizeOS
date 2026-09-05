// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <main/defs.hpp>

namespace krnl
{
	struct PACK ISR_InterruptStackFrame
	{
		uint64_t handlerFlags;
		uint64_t ds, es;
		uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
		uint64_t rdi, rsi, kernelRsp;
		uint64_t rbp, rdx, rcx, rbx, rax;
		uint64_t interrupt, errcode;
		uint64_t rip, cs, rflags, userRsp, ss;
	};

	enum ISR_InterruptHandlerFlags : uint64_t
	{
		// Bit 0: Extra alignment needed? 0: no, 1: yes. Note that this value is only set for the copy of the flags, for the actual flags this bit is reserved
		ISR_HANDLER_FLAG_EXTRA_ALIGNMENT = (1ULL << 0),
	};

	typedef void (*ISRHandler)(ISR_InterruptStackFrame* regs);

	enum ISR_ExceptionVector : uint8_t
	{
		ISR_DIVIDE_BY_ZERO_FAULT                    = 0,
		ISR_DEBUG_EXCEPTION                         = 1,
		ISR_NON_MASKABLE_INTERRUPT_EXCEPTION        = 2,
		ISR_BREAKPOINT_EXCEPTION                    = 3,
		ISR_OVERFLOW_EXCEPTION                      = 4,
		ISR_BOUND_RANGE_EXCEEDED_EXCEPTION          = 5,
		ISR_INVALID_OPCODE_EXCEPTION                = 6,
		ISR_DEVICE_NOT_AVAILABLE_EXCEPTION          = 7,
		ISR_DOUBLE_FAULT                            = 8,
		ISR_COPROCESSOR_SEGMENT_OVERRUN_EXCEPTION   = 9,
		ISR_INVALID_TSS_EXCEPTION                   = 10,
		ISR_SEGMENT_NOT_PRESENT_EXCEPTION           = 11,
		ISR_STACK_SEGMENT_FAULT                     = 12,
		ISR_GENERAL_PROTECTION_FAULT                = 13,
		ISR_PAGE_FAULT                              = 14,
	
		// 15 is reserved
	
		ISR_X87_FLOATING_POINT_EXCEPTION            = 16,
		ISR_ALIGNMENT_CHECK_EXCEPTION               = 17,
		ISR_MACHINE_CHECK_EXCEPTION                 = 18,
		ISR_SIMD_FLOATING_POINT_EXCEPTION           = 19,
		ISR_VIRTUALIZATION_EXCEPTION                = 20,
		ISR_CONTROL_PROTECTION_EXCEPTION            = 21,
	
		// 22-27 are reserved
	
		ISR_HYPERVISOR_INJECTION_EXCEPTION          = 28,
		ISR_VMM_COMMUNICATION_EXCEPTION             = 29,
		ISR_SECURITY_EXCEPTION                      = 30
	
		// 31 is reserved
	};

	class ISR
	{
	public:
		void Initialize();
		void RegisterHandler(int interrupt, ISRHandler handler);
		static ISRHandler handlers[256];
	};
}