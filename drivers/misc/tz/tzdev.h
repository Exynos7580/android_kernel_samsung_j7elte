#ifndef __TZDEV_H__
#define __TZDEV_H__

//#define TZIO_DEBUG

#include <linux/version.h>		/* for linux kernel version information */
#include <linux/workqueue.h>

#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
#if defined(CONFIG_ARCH_APQ8084)
#include <soc/qcom/scm.h>
#elif defined(CONFIG_ARCH_MSM8974)
#include <mach/scm.h>
#endif
#endif

#include "tz_common.h"
#include "tzdev_smc.h"

#define TZDEV_DRIVER_VERSION(a,b,c)	(((a) << 16) | ((b) << 8) | (c))
#define TZDEV_DRIVER_CODE		TZDEV_DRIVER_VERSION(2,0,0)

#define SMC_NO(x)			"" # x
#define SMC(x)				"smc " SMC_NO(x)

/* TODO: Need to check which version should be used */
#define SMC_CURRENT_AARCH SMC_AARCH_32

#define TZDEV_SMC_CONNECT_RAW		0x00000000
#define TZDEV_SMC_COMMMAND_RAW		0x00000002
#define TZDEV_SMC_NWD_DEAD_RAW		0x00000003
#define TZDEV_SMC_GET_EVENT_RAW		0x00000004
#define TZDEV_SMC_NWD_ALIVE_RAW		0x00000005
#define TZDEV_SMC_SHMEM_LIST_REG_RAW	0x00000006
#define TZDEV_SMC_SHMEM_LIST_RLS_RAW	0x00000007
#define TZDEV_SMC_CHECK_VERSION_RAW	0x0000000d
#define TZDEV_SMC_MEM_REG_RAW		0x0000000e
#define TZDEV_SMC_SHUTDOWN_RAW		0x0000000f

#ifdef CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION

#define TZDEV_SMC_MAGIC			0

#define TZDEV_SMC_CONNECT		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_CONNECT_RAW)
#define TZDEV_SMC_COMMMAND		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_COMMMAND_RAW)
#define TZDEV_SMC_NWD_DEAD		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_NWD_DEAD_RAW)
#define TZDEV_SMC_GET_EVENT		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_GET_EVENT_RAW)
#define TZDEV_SMC_NWD_ALIVE		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_NWD_ALIVE_RAW)
#define TZDEV_SMC_SHMEM_LIST_REG	CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_SHMEM_LIST_REG_RAW)
#define TZDEV_SMC_SHMEM_LIST_RLS	CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_SHMEM_LIST_RLS_RAW)
#define TZDEV_SMC_CHECK_VERSION		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_CHECK_VERSION_RAW)
#define TZDEV_SMC_MEM_REG		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_MEM_REG_RAW)
#define TZDEV_SMC_SHUTDOWN		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_CURRENT_AARCH, SMC_TOS0_SERVICE_MASK, TZDEV_SMC_SHUTDOWN_RAW)

#else

#define TZDEV_SMC_MAGIC			(0xE << 16)

#define TZDEV_SMC_CONNECT		TZDEV_SMC_CONNECT_RAW
#define TZDEV_SMC_COMMMAND		TZDEV_SMC_COMMMAND_RAW
#define TZDEV_SMC_NWD_DEAD		TZDEV_SMC_NWD_DEAD_RAW
#define TZDEV_SMC_GET_EVENT		TZDEV_SMC_GET_EVENT_RAW
#define TZDEV_SMC_NWD_ALIVE		TZDEV_SMC_NWD_ALIVE_RAW
#define TZDEV_SMC_SHMEM_LIST_REG	TZDEV_SMC_SHMEM_LIST_REG_RAW
#define TZDEV_SMC_SHMEM_LIST_RLS	TZDEV_SMC_SHMEM_LIST_RLS_RAW
#define TZDEV_SMC_CHECK_VERSION		TZDEV_SMC_CHECK_VERSION_RAW
#define TZDEV_SMC_MEM_REG		TZDEV_SMC_MEM_REG_RAW
#define TZDEV_SMC_SHUTDOWN		TZDEV_SMC_SHUTDOWN_RAW

#endif /* CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION */

#define TZDEV_CONNECT_LOG		0x0
#define TZDEV_CONNECT_AUX		0x1

#define TZDEV_SHMEM_REG			0x0
#define TZDEV_SHMEM_RLS			0x1

/* Define type for exchange with Secure kernel */
#ifdef CONFIG_TZDEV_32BIT_SECURE_KERNEL
typedef	u32	sk_pfn_t;
#else
typedef	u64	sk_pfn_t;
#endif

#ifdef CONFIG_TZLOG
#define TZDEV_LOG_BUF_SIZE		(CONFIG_TZLOG_PG_CNT * PAGE_SIZE - 8)

struct tzio_log_channel {
	unsigned int write_count;
	unsigned int read_count;
	char buffer[TZDEV_LOG_BUF_SIZE];
} __attribute__((__packed__));

void tzio_log_channel_read_debug(void);
#endif

#ifdef CONFIG_TZLOG_POLLING
extern struct delayed_work tzio_log_work;
#endif

#define TZDEV_AUX_BUF_SIZE		PAGE_SIZE

struct tzio_aux_channel {
	char buffer[TZDEV_AUX_BUF_SIZE];
} __attribute__((__packed__));

#define tzdev_smc_cmd(p0, p1, p2, p3)		__tzdev_smc_cmd((unsigned long)(p0), (unsigned long)(p1), (unsigned long)(p2), (unsigned long)(p3))

#define tzdev_smc_connect(mode, phys, nr_pages)	tzdev_smc_cmd(TZDEV_SMC_CONNECT, (mode), (phys), (nr_pages))
#define tzdev_smc_command(tid, pfn)		tzdev_smc_cmd(TZDEV_SMC_COMMMAND, (tid), (pfn), 0x0ul)
#define tzdev_smc_nwd_dead()			tzdev_smc_cmd(TZDEV_SMC_NWD_DEAD, 0x0ul, 0x0ul, 0x0ul)
#define tzdev_smc_nwd_alive()			tzdev_smc_cmd(TZDEV_SMC_NWD_ALIVE, 0x0ul, 0x0ul, 0x0ul)
#define tzdev_smc_get_event(tv_sec, tv_nsec)	tzdev_smc_cmd(TZDEV_SMC_GET_EVENT, (tv_sec), (tv_nsec), 0x0ul)
#define tzdev_smc_shmem_list_reg(id, tot_pfns, write)	tzdev_smc_cmd(TZDEV_SMC_SHMEM_LIST_REG, (id), (tot_pfns), (write))
#define tzdev_smc_shmem_list_rls(id)		tzdev_smc_cmd(TZDEV_SMC_SHMEM_LIST_RLS, (id), 0x0ul, 0x0ul)
#define tzdev_smc_check_version()		tzdev_smc_cmd(TZDEV_SMC_CHECK_VERSION, LINUX_VERSION_CODE, TZDEV_DRIVER_CODE, 0x0ul)
#define tzdev_smc_mem_reg(phys, order)		tzdev_smc_cmd(TZDEV_SMC_MEM_REG, phys, order, 0x0ul)
#define tzdev_smc_shutdown()			tzdev_smc_cmd(TZDEV_SMC_SHUTDOWN, 0x0ul, 0x0ul, 0x0ul)

#ifdef CONFIG_ARM
#define REGISTERS_NAME	"r"
#define ARCH_EXTENSION	".arch_extension sec\n"
#elif defined CONFIG_ARM64
#define REGISTERS_NAME	"x"
#define ARCH_EXTENSION	""
#endif

#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
/* svc_id and cmd_id to call QSEE 3-rd party smc handler */
#define TZ_SVC_EXECUTIVE_EXT		250
#define TZ_CMD_ID_EXEC_SMC_EXT		1

#define SCM_EBUSY			-55

struct tzdev_msm_msg {
	unsigned long p0;
	unsigned long p1;
	unsigned long p2;
	unsigned long p3;
};

struct tzdev_msm_ret_msg {
	unsigned long ret;
};

static inline unsigned long __tzdev_smc_call(unsigned long p0,
					     unsigned long p1,
					     unsigned long p2,
					     unsigned long p3)
{
	struct tzdev_msm_msg msm_msg = {p0, p1, p2, p3};
	struct tzdev_msm_ret_msg ret = {0};
	int rv;

	rv = scm_call(TZ_SVC_EXECUTIVE_EXT, TZ_CMD_ID_EXEC_SMC_EXT,
		      &msm_msg, sizeof(msm_msg),
		      &ret, sizeof(ret));

	if (rv != 0) {
		printk(KERN_ERR "scm_call() failed: %d\n", rv);
		if (rv == SCM_EBUSY)
			rv = -EBUSY;
		return rv;
	}

	return ret.ret;
}
#else /* defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM) */
static inline unsigned long __tzdev_smc_call(unsigned long p0,
					     unsigned long p1,
					     unsigned long p2,
					     unsigned long p3)
{
	register unsigned long _a0 __asm__(REGISTERS_NAME "0") = p0 | TZDEV_SMC_MAGIC;
	register unsigned long _a1 __asm__(REGISTERS_NAME "1") = p1;
	register unsigned long _a2 __asm__(REGISTERS_NAME "2") = p2;
	register unsigned long _a3 __asm__(REGISTERS_NAME "3") = p3;
	register unsigned long _r0 __asm__(REGISTERS_NAME "0");

	__asm__ __volatile__(ARCH_EXTENSION SMC(0): "=r"(_r0) :
			     "0"(_a0),"r"(_a1), "r"(_a2), "r"(_a3) : "memory");

	return _r0;
}
#endif /* defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM) */

static inline long __tzdev_smc_cmd(unsigned long p0,
				   unsigned long p1,
				   unsigned long p2,
				   unsigned long p3)
{
	unsigned long ret;
#ifdef TZIO_DEBUG
	printk(KERN_ERR "tzdev_smc_cmd: p0=0x%lx, p1=0x%lx, p2=0x%lx, p3=0x%lx\n",
			p0, p1, p2, p3);
#endif

#ifdef CONFIG_TZLOG_POLLING
	schedule_delayed_work(&tzio_log_work,
			      CONFIG_TZLOG_POLLING_PERIOD * HZ / MSEC_PER_SEC);
#endif

	ret = __tzdev_smc_call(p0, p1, p2, p3);

#ifdef CONFIG_TZLOG_POLLING
	cancel_delayed_work_sync(&tzio_log_work);
#endif
#ifdef CONFIG_TZLOG
	tzio_log_channel_read_debug();
#endif

	return ret;
}

#endif /* __TZDEV_H__ */
