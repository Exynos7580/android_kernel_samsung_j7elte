
#ifndef _IMM_VIB_H
#define _IMM_VIB_H

#include <linux/miscdevice.h>
#include <linux/mutex.h>

/* DO NOT CHANGE - this is auto-generated */
#define VERSION_STR " v3.4.55.8\n"

#define VERSION_STR_LEN 16
#define WATCHDOG_TIMEOUT    10  /* 10 timer cycles = 50ms */
#define MODULE_NAME                         "tspdrv"
#define TSPDRV                              "/dev/"MODULE_NAME
#define IMM_VIB_MAGIC_NUMBER                 0x494D4D52
#define IMM_VIB_STOP_KERNEL_TIMER            _IO(IMM_VIB_MAGIC_NUMBER & 0xFF, 1)
#define IMM_VIB_ENABLE_AMP                   _IO(IMM_VIB_MAGIC_NUMBER & 0xFF, 3)
#define IMM_VIB_DISABLE_AMP                  _IO(IMM_VIB_MAGIC_NUMBER & 0xFF, 4)
#define IMM_VIB_GET_NUM_ACTUATORS            _IO(IMM_VIB_MAGIC_NUMBER & 0xFF, 5)
#define IMM_VIB_NAME_SIZE			64
#define SPI_HEADER_SIZE                     3   /* DO NOT CHANGE - SPI buffer header size */
#define IMM_VIB_OUTPUT_SAMPLE_SIZE             50  /* DO NOT CHANGE - maximum number of samples */
#define IMM_VIB_FAIL						-4	/* Generic error */
#define NUM_ACTUATORS				2
#define SPI_BUFFER_SIZE (NUM_ACTUATORS * (IMM_VIB_OUTPUT_SAMPLE_SIZE + SPI_HEADER_SIZE))

struct samples_data {
	u8 index;
	u8 bit_detpth;
	u8 size;
	u8 data[IMM_VIB_OUTPUT_SAMPLE_SIZE];
};

struct actuator_data {
	struct samples_data samples[2];
	s8 playing;
	u8 output;
};

struct imm_vib_dev {
	void (*set_force)(void *_data, u8 index, int nforce);
	void (*chip_en)(void *_data, bool en);
	struct actuator_data actuator[NUM_ACTUATORS];
	struct hrtimer timer;
	struct wake_lock wlock;
	struct miscdevice miscdev;
	struct semaphore *sem;
	ktime_t timer_ms;
	size_t cch_device_name;
	bool is_playing;
	bool stop_requested;
	bool amp_enabled;
	bool timer_started;
	int motor_enabled;
	int watchdog_cnt;
	int num_actuators;
	char write_buff[SPI_BUFFER_SIZE];
	char device_name[(IMM_VIB_NAME_SIZE + VERSION_STR_LEN) * NUM_ACTUATORS];
	void *private_data;
};

extern int imm_vib_register(struct imm_vib_dev *dev);
extern void imm_vib_unregister(struct imm_vib_dev *dev);

#endif  /* _IMM_VIB_H */
