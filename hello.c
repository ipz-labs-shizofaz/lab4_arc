/*
 * hello.c - kernel module for lab 4
 * prints "Hello, world!" N times (module parameter)
 * records ktime for each print in a list and prints times at unload
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/ktime.h>
#include <linux/errno.h>
#include <linux/stat.h>

MODULE_AUTHOR("Your Name <youremail@example.com>");
MODULE_DESCRIPTION("Hello module (lab 4): prints Hello N times, records times");
MODULE_LICENSE("Dual BSD/GPL");

/* Module parameter: how many times to print Hello (unsigned int) */
static uint howmany = 1;
module_param(howmany, uint, S_IRUGO);
MODULE_PARM_DESC(howmany, "Number of times to print 'Hello, world!' (uint). Default=1");

/* list element: contains list_head and ktime */
struct hello_event {
	struct list_head list;
	ktime_t time;
};

/* static list head */
static LIST_HEAD(hello_events);

/* init */
static int __init hello_init(void)
{
	unsigned int i;
	struct hello_event *evt;

	/* validate parameter */
	if (howmany > 10) {
		printk(KERN_ERR "hello: howmany=%u > 10 -> refusing to load (EINVAL)\n", howmany);
		return -EINVAL;
	}

	if (howmany == 0 || (howmany >= 5 && howmany <= 10)) {
		printk(KERN_WARNING "hello: howmany=%u is special (0 or 5..10) — continuing with warning\n", howmany);
	}

	for (i = 0; i < howmany; i++) {
		/* allocate event */
		evt = kmalloc(sizeof(*evt), GFP_KERNEL);
		if (!evt) {
			/* allocation failure: clean up allocated entries and fail */
			struct hello_event *pos, *n;
			printk(KERN_ERR "hello: kmalloc failed on iteration %u\n", i);
			list_for_each_entry_safe(pos, n, &hello_events, list) {
				list_del(&pos->list);
				kfree(pos);
			}
			return -ENOMEM;
		}

		evt->time = ktime_get();
		INIT_LIST_HEAD(&evt->list);
		list_add_tail(&evt->list, &hello_events);

		printk(KERN_ALERT "Hello, world!\n");
	}

	return 0;
}

/* exit - print times in nanoseconds, free list */
static void __exit hello_exit(void)
{
	struct hello_event *pos, *n;

	list_for_each_entry_safe(pos, n, &hello_events, list) {
		/* ktime_to_ns returns s64, print with %lld */
		printk(KERN_INFO "hello: event time = %lld ns\n", (long long)ktime_to_ns(pos->time));
		list_del(&pos->list);
		kfree(pos);
	}

	printk(KERN_INFO "hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
