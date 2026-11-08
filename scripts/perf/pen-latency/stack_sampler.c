// SPDX-License-Identifier: GPL-3.0-or-later
//
// LD_PRELOAD library that samples the main-thread call stack of a process
// named krita every SAMPLE_PERIOD_US (default 2000) with SIGPROF, starting
// SAMPLE_DELAY seconds (default 5) after start, for up to SAMPLE_SECONDS
// (default 115), and writes SAMPLE_OUT when sampling ends or on SIGTERM.
// It shows where the GUI thread spends time between latency-probe markers,
// including time spent waiting, which on-CPU profilers miss.
//   s,<CLOCK_MONOTONIC ns>,<pc>;<pc>;...
//   m,<pc>,<module>,<offset>,<nearest exported symbol>
//
// Build: cc -O2 -shared -fPIC stack_sampler.c -ldl -lpthread -o stack_sampler.so

#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#define DEPTH 40
#define MAXS 60000
typedef struct { uint64_t t; int n; void *pc[DEPTH]; } Sample;
static Sample *samples; static atomic_int count; static pid_t main_tid; static atomic_int stop_flag, done_flag;
static int cmpp(const void*a,const void*b){uintptr_t x=*(uintptr_t*)a,y=*(uintptr_t*)b;return x<y?-1:x>y;}
static void on_term(int sig){atomic_store(&stop_flag,1);for(int i=0;i<180&&!atomic_load(&done_flag);i++){struct timespec q={0,10000000};nanosleep(&q,0);}signal(sig,SIG_DFL);raise(sig);}
static uint64_t now(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000000ull+ts.tv_nsec;}
static void handler(int sig,siginfo_t*si,void*uc){(void)sig;(void)si;(void)uc;int i=atomic_fetch_add(&count,1);if(i>=MAXS)return;samples[i].t=now();samples[i].n=backtrace(samples[i].pc,DEPTH);}
static void *run(void*arg){(void)arg;const char*o=getenv("SAMPLE_OUT");double d=getenv("SAMPLE_DELAY")?atof(getenv("SAMPLE_DELAY")):5,s=getenv("SAMPLE_SECONDS")?atof(getenv("SAMPLE_SECONDS")):115;long p=getenv("SAMPLE_PERIOD_US")?atol(getenv("SAMPLE_PERIOD_US")):2000;
 struct timespec w={(time_t)d,(long)((d-(time_t)d)*1e9)};nanosleep(&w,0);uint64_t end=now()+(uint64_t)(s*1e9);
 while(now()<end&&atomic_load(&count)<MAXS&&!atomic_load(&stop_flag)){syscall(SYS_tgkill,getpid(),main_tid,SIGPROF);struct timespec q={0,p*1000};nanosleep(&q,0);}
 struct timespec q={0,50000000};nanosleep(&q,0);int n=atomic_load(&count);if(n>MAXS)n=MAXS;FILE*f=fopen(o,"w");if(!f)return 0;
 for(int i=0;i<n;i++){fprintf(f,"s,%llu,",(unsigned long long)samples[i].t);for(int k=0;k<samples[i].n;k++)fprintf(f,"%s%p",k?";":"",samples[i].pc[k]);fputc(10,f);}
 size_t m=0;uintptr_t*u=malloc(sizeof(uintptr_t)*(size_t)n*DEPTH+1);for(int i=0;i<n;i++)for(int k=0;k<samples[i].n;k++)u[m++]=(uintptr_t)samples[i].pc[k];qsort(u,m,sizeof *u,cmpp);
 for(size_t i=0;i<m;i++){if(i&&u[i]==u[i-1])continue;Dl_info di;void*a=(void*)u[i];if(dladdr(a,&di)&&di.dli_fname)fprintf(f,"m,%p,%s,%lx,%s\n",a,di.dli_fname,(unsigned long)((char*)a-(char*)di.dli_fbase),di.dli_sname?di.dli_sname:"?");}
 fclose(f);atomic_store(&done_flag,1);return 0;}
extern char *program_invocation_short_name;
__attribute__((constructor)) static void init(void){if(!getenv("SAMPLE_OUT")||strcmp(program_invocation_short_name,"krita"))return;main_tid=syscall(SYS_gettid);samples=calloc(MAXS,sizeof(Sample));void*b[2];backtrace(b,2);
 struct sigaction sa;memset(&sa,0,sizeof sa);sa.sa_sigaction=handler;sa.sa_flags=SA_SIGINFO|SA_RESTART;sigaction(SIGPROF,&sa,0);signal(SIGTERM,on_term);pthread_t t;pthread_create(&t,0,run,0);pthread_detach(t);}
