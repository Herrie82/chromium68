// Copyright 2018 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// namespace crashpad {
// void CaptureContext(ucontext_t* context);
// }  // namespace crashpad

// The type name for a ucontext_t varies by libc implementation and version.
// Bionic and glibc 2.25 typedef ucontext_t from struct ucontext. glibc 2.26+
// typedef ucontext_t from struct ucontext_t. Alias the symbol names to maintain
// compatibility with both possibilities.
#define CAPTURECONTEXT_SYMBOL _ZN8crashpad14CaptureContextEP10ucontext_t
#define CAPTURECONTEXT_SYMBOL2 _ZN8crashpad14CaptureContextEP8ucontext

  .text
  .globl CAPTURECONTEXT_SYMBOL
  .globl CAPTURECONTEXT_SYMBOL2
#if defined(__i386__) || defined(__x86_64__)
  .balign 16, 0x90
#elif defined(__arm__) || defined(__aarch64__)
  .balign 4, 0x0
#endif

CAPTURECONTEXT_SYMBOL:
CAPTURECONTEXT_SYMBOL2:

#if defined(__i386__)

  .cfi_startproc

  pushl %ebp
  .cfi_def_cfa_offset 8
  .cfi_offset %ebp, -8
  movl %esp, %ebp
  .cfi_def_cfa_register %ebp

  // Note that 16-byte stack alignment is not maintained because this function
  // does not call out to any other.

  // pushfl first, because some instructions (but probably none used here)
  // affect %eflags. %eflags will be in -4(%ebp).
  pushfl

  // Save the original value of %eax, and use %eax to hold the ucontext_t*
  // argument. The original value of %eax will be in -8(%ebp).
  pushl %eax
  movl 8(%ebp), %eax

  // Save the original value of %ecx, and use %ecx as a scratch register.
  pushl %ecx

  // The segment registers are 16 bits wide, but mcontext_t declares them
  // as unsigned 32-bit values, so zero the top half.
  xorl %ecx, %ecx
  movw %gs, %cx
  movl %ecx, 0x14(%eax)  // context->uc_mcontext.xgs
  movw %fs, %cx
  movl %ecx, 0x18(%eax)  // context->uc_mcontext.xfs
  movw %es, %cx
  movl %ecx, 0x1c(%eax)  // context->uc_mcontext.xes
  movw %ds, %cx
  movl %ecx, 0x20(%eax)  // context->uc_mcontext.xds

  // General-purpose registers whose values haven’t changed can be captured
  // directly.
  movl %edi, 0x24(%eax)  // context->uc_mcontext.edi
  movl %esi, 0x28(%eax)  // context->uc_mcontext.esi

  // The original %ebp was saved on the stack in this function’s prologue.
  movl (%ebp), %ecx
  movl %ecx, 0x2c(%eax)  // context->uc_mcontext.ebp

  // %esp was saved in %ebp in this function’s prologue, but the caller’s %esp
  // is 8 more than this value: 4 for the original %ebp saved on the stack in
  // this function’s prologue, and 4 for the return address saved on the stack
  // by the call instruction that reached this function.
  leal 8(%ebp), %ecx
  movl %ecx, 0x30(%eax)  // context->uc_mcontext.esp

  // More general-purpose registers
  movl %ebx, 0x34(%eax)  // context->uc_mcontext.ebx
  movl %edx, 0x38(%eax)  // context->uc_mcontext.edx

  // The original %ecx was saved on the stack above.
  movl -12(%ebp), %ecx
  movl %ecx, 0x3c(%eax)  // context->uc_mcontext.ecx

  // The original %eax was saved on the stack above.
  movl -8(%ebp), %ecx
  movl %ecx, 0x40(%eax)  // context->uc_mcontext.eax

  // trapno and err are unused so zero them out.
  xorl %ecx, %ecx
  movl %ecx, 0x44(%eax)  // context->uc_mcontext.trapno
  movl %ecx, 0x48(%eax)  // context->uc_mcontext.err

  // %eip can’t be accessed directly, but the return address saved on the stack
  // by the call instruction that reached this function can be used.
  movl 4(%ebp), %ecx
  movl %ecx, 0x4c(%eax)  // context->uc_mcontext.eip

  // More segment registers
  xorl %ecx, %ecx
  movw %cs, %cx
  movl %ecx, 0x50(%eax)  // context->uc_mcontext.xcs

  // The original %eflags was saved on the stack above.
  movl -4(%ebp), %ecx
  movl %ecx, 0x54(%eax)  // context->uc_mcontext.eflags

  // uesp is unused so zero it out.
  xorl %ecx, %ecx
  movl %ecx, 0x58(%eax)  // context->uc_mcontext.uesp

  // The last segment register.
  movw %ss, %cx
  movl %ecx, 0x5c(%eax)  // context->uc_mcontext.xss

  // TODO(jperaza): save floating-point registers.
  xorl %ecx, %ecx
  movl %ecx, 0x60(%eax)  // context->uc_mcontext.fpregs

  // Clean up by restoring clobbered registers, even those considered volatile
  // by the ABI, so that the captured context represents the state at this
  // function’s exit.
  popl %ecx
  popl %eax
  popfl

  popl %ebp

  ret

  .cfi_endproc

#elif defined(__x86_64__)

  .cfi_startproc

  pushq %rbp
  .cfi_def_cfa_offset 16
  .cfi_offset %rbp, -16
  movq %rsp, %rbp
  .cfi_def_cfa_register %rbp

  // Note that 16-byte stack alignment is not maintained because this function
  // does not call out to any other.

  // pushfq first, because some instructions (but probably none used here)
  // affect %rflags. %rflags will be in -8(%rbp).
  pushfq

  // General-purpose registers whose values haven’t changed can be captured
  // directly.
  movq %r8, 0x28(%rdi)  // context->uc_mcontext.r8
  movq %r9, 0x30(%rdi)  // context->uc_mcontext.r9
  movq %r10, 0x38(%rdi)  // context->uc_mcontext.r10
  movq %r11, 0x40(%rdi)  // context->uc_mcontext.r11
  movq %r12, 0x48(%rdi)  // context->uc_mcontext.r12
  movq %r13, 0x50(%rdi)  // context->uc_mcontext.r13
  movq %r14, 0x58(%rdi)  // context->uc_mcontext.r14
  movq %r15, 0x60(%rdi)  // context->uc_mcontext.r15

  // Because of the calling convention, there’s no way to recover the value of
  // the caller’s %rdi as it existed prior to calling this function. This
  // function captures a snapshot of the register state at its return, which
  // involves %rdi containing a pointer to its first argument. Callers that
  // require the value of %rdi prior to calling this function should obtain it
  // separately. For example:
  //   uint64_t rdi;
  //   asm("movq %%rdi, %0" : "=m"(rdi));
  movq %rdi, 0x68(%rdi)  // context->uc_mcontext.rdi

  movq %rsi, 0x70(%rdi)  // context->uc_mcontext.rsi

  // Use %r8 as a scratch register now that it has been saved.
  // The original %rbp was saved on the stack in this function’s prologue.
  movq (%rbp), %r8
  movq %r8, 0x78(%rdi)  // context->uc_mcontext.rbp

  // Save the remaining general-purpose registers.
  movq %rbx, 0x80(%rdi)  // context->uc_mcontext.rbx
  movq %rdx, 0x88(%rdi)  // context->uc_mcontext.rdx
  movq %rax, 0x90(%rdi)  // context->uc_mcontext.rax
  movq %rcx, 0x98(%rdi)  // context->uc_mcontext.rcx

  // %rsp was saved in %rbp in this function’s prologue, but the caller’s %rsp
  // is 16 more than this value: 8 for the original %rbp saved on the stack in
  // this function’s prologue, and 8 for the return address saved on the stack
  // by the call instruction that reached this function.
  leaq 16(%rbp), %r8
  movq %r8, 0xa0(%rdi)  // context->uc_mcontext.rsp

  // %rip can’t be accessed directly, but the return address saved on the stack
  // by the call instruction that reached this function can be used.
  movq 8(%rbp), %r8
  movq %r8, 0xa8(%rdi)  // context->uc_mcontext.rip

  // The original %rflags was saved on the stack above.
  movq -8(%rbp), %r8
  movq %r8, 0xb0(%rdi)  // context->uc_mcontext.eflags

  // Save the segment registers
  movw %cs, 0xb8(%rdi)  // context->uc_mcontext.cs
  movw %gs, 0xba(%rdi)  // context->uc_mcontext.gs
  movw %fs, 0xbc(%rdi)  // context->uc_mcontext.fs

  xorw %ax, %ax
  movw %ax, 0xbe(%rdi)  // context->uc_mcontext.padding

  // Zero out the remainder of the unused pseudo-registers
  xorq %r8, %r8
  movq %r8, 0xc0(%rdi)  // context->uc_mcontext.err
  movq %r8, 0xc8(%rdi)  // context->uc_mcontext.trapno
  movq %r8, 0xd0(%rdi)  // context->uc_mcontext.oldmask
  movq %r8, 0xd8(%rdi)  // context->uc_mcontext.cr2

  // TODO(jperaza): save floating-point registers.
  movq %r8, 0xe0(%rdi)  // context->uc_mcontext.fpregs

  // Clean up by restoring clobbered registers, even those considered volatile
  // by the ABI, so that the captured context represents the state at this
  // function’s exit.
  movq 0x90(%rdi), %rax
  movq 0x28(%rdi), %r8

  popfq

  popq %rbp

  ret

  .cfi_endproc

#elif defined(__arm__)

  // The original r0 can't be recovered.
  str r0, [r0, #0x20]

  // Now advance r0 to point to the register array.
  add r0, r0, #0x24

  // Save registers r1-r12 at context->uc_mcontext.regs[i].
  stm r0, {r1-r12}

  // Restore r0.
  sub r0, r0, #0x24

  // Save named general purpose registers.
  str FP, [r0, #0x4c]  // context->uc_mcontext.fp
  str IP, [r0, #0x50]  // context->uc_mcontext.ip
  str SP, [r0, #0x54]  // context->uc_mcontext.sp

  // The original LR can't be recovered.
  str LR, [r0, #0x58]  // context->uc_mcontext.lr

  // The link register holds the return address for this function.
  str LR, [r0, #0x5c]  // context->uc_mcontext.pc

  // Use r1 as a scratch register.

  // CPSR is a deprecated synonym for APSR.
  mrs r1, APSR
  str r1, [r0, #0x60]  // context->uc_mcontext.cpsr

  // Zero out unused fields.
  mov r1, #0x0
  str r1, [r0, #0x14]  // context->uc_mcontext.trap_no
  str r1, [r0, #0x18]  // context->uc_mcontext.error_code
  str r1, [r0, #0x1c]  // context->uc_mcontext.oldmask
  str r1, [r0, #0x64]  // context->uc_mcontext.fault_address

  // Restore r1.
  ldr r1, [r0, #0x24]

  // TODO(jperaza): save floating-point registers.

  mov PC, LR

#elif defined(__aarch64__)

  // Zero out fault_address, which is unused.
  str xzr, [x0, #0xb0]  // context->uc_mcontext.fault_address

  // Save general purpose registers in context->uc_mcontext.regs[i].
  // The original x0 can't be recovered.
  stp x0, x1, [x0, #0xb8]
  stp x2, x3, [x0, #0xc8]
  stp x4, x5, [x0, #0xd8]
  stp x6, x7, [x0, #0xe8]
  stp x8, x9, [x0, #0xf8]
  stp x10, x11, [x0, #0x108]
  stp x12, x13, [x0, #0x118]
  stp x14, x15, [x0, #0x128]
  stp x16, x17, [x0, #0x138]
  stp x18, x19, [x0, #0x148]
  stp x20, x21, [x0, #0x158]
  stp x22, x23, [x0, #0x168]
  stp x24, x25, [x0, #0x178]
  stp x26, x27, [x0, #0x188]
  stp x28, x29, [x0, #0x198]

  // The original LR can't be recovered.
  str x30, [x0, #0x1a8]

  // Use x1 as a scratch register.
  mov x1, SP
  str x1, [x0, #0x1b0] // context->uc_mcontext.sp

  // The link register holds the return address for this function.
  str x30, [x0, #0x1b8]  // context->uc_mcontext.pc

  // NZCV, pstate, and CPSR are synonyms.
  mrs x1, NZCV
  str x1, [x0, #0x1c0]  // context->uc_mcontext.pstate

  // Restore x1 from the saved context.
  ldr x1, [x0, #0xc0]

  // TODO(jperaza): save floating-point registers.

  ret

#endif  // __i386__
