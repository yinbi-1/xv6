#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

void main();
void timerinit();

// entry.S needs one stack per CPU.
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];

// a scratch area per CPU for machine-mode timer interrupts.
uint64 timer_scratch[NCPU][5];

// assembly code in kernelvec.S for machine-mode timer interrupt.
extern void timervec();

// entry.S jumps here in machine mode on stack0.
void
start()
{
  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus(); // 读取 mstatus 暂存器
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S; // 切换到supervisor mode
  w_mstatus(x);

  // set M Exception Program Counter to main, for mret.
  // requires gcc -mcmodel=medany
  w_mepc((uint64)main); // 该暂存器用途为当trap 发生时所使用的Program counter

  // disable paging for now.
  w_satp(0); // satp暂存器表示是否需要pagging，0表示禁用

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff); // 表示当interrupts (中断) 或是exceptions (例外)
  w_mideleg(0xffff); //  发生时，不会在machine mode 底下进行处理，而是由supervisor mode 进行处理。
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);
  /*  sie 暂存器，用于决定是否interrupt (中断)，对 sie 暂存器写入一些值，
   并且将它存放回 sie 暂存器中，而这一些值表示允许在supervisor mode 底下，
   I/O 装置(外部装置externel) interrupt，timer interrupt，
   software interrupt。SEIE 为Supervisor mode externel interrupt enable 的缩写。*/
  
  // configure Physical Memory Protection to give supervisor mode
  // access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffull);
  w_pmpcfg0(0xf);

  // ask for clock interrupts.
  timerinit(); // 为了后续的timer interrupt。

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);
  /*读取 mhartid 暂存器并将值写入到 tp 暂存器中，
  tp暂存器存放thread pointer。这决定等一下程式码会在哪一个处理器核心上执行。*/

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
  // 1. 从 mepc 指向的内存地址开始执行
  // 更新mstatus寄存器
}

// arrange to receive timer interrupts.
// they will arrive in machine mode at
// at timervec in kernelvec.S,
// which turns them into software interrupts for
// devintr() in trap.c.
void
timerinit()
{
  // each CPU has a separate source of timer interrupts.
  int id = r_mhartid(); // 读取核心编号，得到目前在哪一个核心上面执行

  // ask the CLINT for a timer interrupt.
  int interval = 1000000; // cycles; about 1/10th second in qemu.
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;

  // prepare information in scratch[] for timervec.
  // scratch[0..2] : space for timervec to save registers.
  // scratch[3] : address of CLINT MTIMECMP register.
  // scratch[4] : desired interval (in cycles) between timer interrupts.
  uint64 *scratch = &timer_scratch[id][0];
  scratch[3] = CLINT_MTIMECMP(id);
  scratch[4] = interval;
  w_mscratch((uint64)scratch);

  // set the machine-mode trap handler.
  w_mtvec((uint64)timervec);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  // enable machine-mode timer interrupts.
  w_mie(r_mie() | MIE_MTIE);
}
