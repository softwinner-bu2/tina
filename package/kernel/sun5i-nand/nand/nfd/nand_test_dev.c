
#define _NAND_TEST_DEV_C_

#include "nand_type.h"
#include "nand_blk.h"
#include "nand_dev.h"

extern struct _nand_info* p_nand_info;
extern struct kobj_type ktype;
extern struct _nftl_blk nftl_blk_head;

extern struct _nand_partition* build_nand_partition(struct _nand_phy_partition* phy_partition);
extern void add_nftl_blk_list(struct _nftl_blk*head,struct _nftl_blk *nftl_blk);
extern uint16 get_partitionNO(struct _nand_phy_partition* phy_partition);
extern struct _nand_phy_partition* get_head_phy_partition_from_nand_info(struct _nand_info*nand_info);
extern struct _nand_phy_partition* get_next_phy_partition(struct _nand_phy_partition* phy_partition);
extern void NAND_Interrupt(__u32 nand_index);
extern __u32 NAND_GetCurrentCH(void);
extern int nand_ftl_exit(void);
extern unsigned int nftl_read(unsigned int start_sector,unsigned int len,unsigned char *buf);
extern unsigned int nftl_write(unsigned int start_sector,unsigned int len,unsigned char *buf);
extern unsigned int nftl_flush_write_cache(void);
extern int nand_ftl_exit(void);

extern uint32 gc_all(struct _nftl_zone* zone);
extern uint32 gc_one(struct _nftl_zone* zone);
extern void print_nftl_zone(struct _nftl_zone * zone);
extern void print_free_list(struct _nftl_zone* zone);
extern void print_block_invalid_list(struct _nftl_zone* zone);
extern uint32 nftl_set_zone_test(void * _zone,uint32 num);

static ssize_t nand_test_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count);
static ssize_t nand_test_show(struct kobject *kobject,struct attribute *attr, char *buf);
void obj_test_release(struct kobject *kobject);

int add_nand_for_test(struct _nand_phy_partition* phy_partition);
int build_all_nftl(struct _nand_info*nand_info);

#define NFTL_SCHEDULE_TIMEOUT  (HZ >> 2)
#define NFTL_FLUSH_DATA_TIME	 1

struct attribute prompt_attr = {
    .name = "nand_debug",
    .mode = S_IRWXUGO
};

static struct attribute *def_attrs[] = {
    &prompt_attr,
    NULL
};


struct sysfs_ops obj_test_sysops =
{
    .show =  nand_test_show,
    .store = nand_test_store
};

struct kobj_type ktype =
{
    .release = obj_test_release,
    .sysfs_ops=&obj_test_sysops,
    .default_attrs=def_attrs
};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_test_thread(void *arg)
{
    struct _nftl_blk *nftl_blk = arg;
    unsigned long time;

    nftl_blk->time_flush = NFTL_FLUSH_DATA_TIME * HZ;
    //nftl_blk->time_flush = 30 * HZ;
    nftl_blk->time_flush = HZ;

    while (!kthread_should_stop()) {

        mutex_lock(nftl_blk->blk_lock);

        if(nftl_get_zone_write_cache_nums(nftl_blk->nftl_zone) > 32){
            time = jiffies;
           //if (time_after(time,nftl_blk->time + nftl_blk->time_flush)){
                nftl_blk->flush_write_cache(nftl_blk,8);
           //}
        }
//        else if(nftl_get_zone_write_cache_nums(nftl_blk->nftl_zone) > 200)
//        {
//           time = jiffies;
//           if (time_after(time,nftl_blk->time + nftl_blk->time_flush)){
//                nftl_blk->flush_write_cache(nftl_blk,6);
//           }
//        }
//        else if(nftl_get_zone_write_cache_nums(nftl_blk->nftl_zone) > 100)
//        {
//            time = jiffies;
//           if (time_after(time,nftl_blk->time + nftl_blk->time_flush)){
//                nftl_blk->flush_write_cache(nftl_blk,8);
//           }
//        }
        else
        {
            time = jiffies;
           if (time_after(time,nftl_blk->time + nftl_blk->time_flush + HZ)){
                nftl_blk->flush_write_cache(nftl_blk,2);
           }
        }

//#if  SUPPORT_WEAR_LEVELING
//        if(do_static_wear_leveling(nftl_blk->nftl_zone) != 0){
//            printk("nftl_thread do_static_wear_leveling error!\n");
//        }
//#endif

        if(garbage_collect(nftl_blk->nftl_zone) != 0){
            printk("nftl_thread garbage_collect error!\n");
        }

        if(do_prio_gc(nftl_blk->nftl_zone) != 0){
            printk("nftl_thread do_prio_gc error!\n");
        }

        mutex_unlock(nftl_blk->blk_lock);
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(NFTL_SCHEDULE_TIMEOUT);
    }

    nftl_blk->nftl_thread = (void*)NULL;
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
#ifdef __LINUX_NAND_SUPPORT_INT__

spinlock_t     nand_int_test_lock;

static irqreturn_t nand_interrupt_ch(int irq, void *dev_id)
{
    unsigned long iflags;
    __u32 nand_index;

    //printk("nand_interrupt_ch0!\n");
    spin_lock_irqsave(&nand_int_test_lock, iflags);

    nand_index = NAND_GetCurrentCH();
    if(nand_index!=0)
    {
        //printk(" ch %d int in ch0\n", nand_index);
    }
    else
    {
        NAND_Interrupt(nand_index);
    }
    spin_unlock_irqrestore(&nand_int_test_lock, iflags);

    return IRQ_HANDLED;
}

#endif

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_interrupt_init(void)
{
#ifdef __LINUX_NAND_SUPPORT_INT__

    char * dev_name = "nand_dev";
    char * dev_id = "nand_id";

    unsigned long irqflags_ch0, irqflags_ch1;
    //printk("[NAND] nand driver version: 0x%x 0x%x, support int! \n", NAND_VERSION_0,NAND_VERSION_1);

    spin_lock_init(&nand_int_test_lock);
    irqflags_ch0 = IRQF_DISABLED;
    irqflags_ch1 = IRQF_DISABLED;

    if (request_irq(SW_INT_IRQNO_NAND, nand_interrupt_ch, IRQF_DISABLED, dev_name, &dev_id))
    {
        printk("nand interrupte ch0 irqno: %d register error\n", SW_INT_IRQNO_NAND);
        return -EAGAIN;
    }
    else
    {
        //printk("nand interrupte ch0 irqno: %d register ok\n", AW_IRQ_NAND0);
    }

#endif
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int build_all_nftl(struct _nand_info*nand_info)
{
	struct _nand_phy_partition*  phy_partition;

	phy_partition = get_head_phy_partition_from_nand_info(nand_info);

    nftl_blk_head.nftl_blk_next = NULL;
	while(phy_partition != NULL)
	{
		if(add_nand_for_test(phy_partition) != 0)
		{
		    printk("build_all_nftl fail!\n");
			return 1;
		}
		phy_partition = get_next_phy_partition(phy_partition);
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_nand_for_test(struct _nand_phy_partition* phy_partition)
{
	int i;
    __u32 cur_offset = 0;
    struct _nftl_blk *nftl_blk;
    struct _nand_dev *nand_dev;
	struct _nand_disk* disk;
	struct _nand_disk* head_disk;
	struct nand_kobject* nand_kobj;
	uint16 PartitionNO;

    PartitionNO = get_partitionNO(phy_partition);

    nftl_blk = kmalloc(sizeof(struct _nftl_blk), GFP_KERNEL);
    if (!nftl_blk)
	{
	    printk("nftl_blk kmalloc fail!\n");
		return 1;
	}
    nftl_blk->nand = build_nand_partition(phy_partition);

    if (nftl_initialize(nftl_blk,PartitionNO))
	{
        printk("nftl_initialize failed\n");
        return 1;
    }

    nftl_blk->blk_lock = kmalloc(sizeof(struct mutex), GFP_KERNEL);
    if (!nftl_blk->blk_lock)
	{
	    printk("blk_lock kmalloc fail!\n");
		return 1;
	}
    mutex_init(nftl_blk->blk_lock);

	nftl_blk->nftl_thread = kthread_run(nftl_test_thread, nftl_blk, "%sd", "nftl");
    if (IS_ERR(nftl_blk->nftl_thread))
    {
        printk("nftl_thread kthread_run fail!\n");
        return 1;
    }

    add_nftl_blk_list(&nftl_blk_head,nftl_blk);

    nand_kobj = kzalloc(sizeof(struct nand_kobject), GFP_KERNEL);
    if (!nand_kobj)
	{
	    printk("nand_kobj kzalloc fail!\n");
		return 1;
	}
	nand_kobj->nftl_blk = nftl_blk;
    if(kobject_init_and_add(&nand_kobj->kobj,&ktype,NULL,"nand_driver%d",PartitionNO) != 0 ) {
    	printk("init nand sysfs fail!\n");
	  	return 1;
	}

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_driver_test_init(void)
{
    int ret = 0;
    struct _nand_info* p_nand_info2;

    ret = nand_interrupt_init();
    if(ret != 0)
    {
        printk("nand_driver_test_init fail1!\n");
        return ret;
    }

    p_nand_info2 = NandHwInit();
    if(p_nand_info2 == NULL)
    {
        printk("nand_driver_test_init fail2!\n");
        NandHwExit();
        return 1;
    }

    ret = nand_info_init(p_nand_info2,0,8,NULL);
    if(ret != 0)
    {
        printk("nand_driver_test_init fail3!\n");
        return ret;
    }

    ret = build_all_nftl(p_nand_info2);
    if(ret != 0)
    {
        printk("nand_driver_test_init fail4!\n");
        nand_ftl_exit();
        return ret;
    }
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_driver_test_exit(void)
{
    nftl_flush_write_cache();
    nand_ftl_exit();
    NandHwExit();
    return 0;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nftl_test_read(uint32 start_sector,uint32 len,unsigned char *buf)
{
    int ret = 0;
    struct _nftl_blk *nftl_blk;
    nftl_blk = nftl_blk_head.nftl_blk_next;

    if(len == 0)
        return 0;

    while(start_sector >= nftl_blk->nftl_logic_size)
    {
        start_sector -= nftl_blk->nftl_logic_size;
        nftl_blk = nftl_blk->nftl_blk_next;
        if(nftl_blk == NULL)
        {
            printk("[NE]parameter error %d,%d !\n",start_sector,len);
            return 1;
        }
    }

    mutex_lock(nftl_blk->blk_lock);
    ret = nftl_blk->read_data(nftl_blk,start_sector,len,buf);
    mutex_unlock(nftl_blk->blk_lock);

    return  ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nftl_test_write(uint32 start_sector,uint32 len,unsigned char *buf)
{
    int ret = 0;
    struct _nftl_blk *nftl_blk;
    nftl_blk = nftl_blk_head.nftl_blk_next;

    if(len == 0)
        return 0;

    while(start_sector >= nftl_blk->nftl_logic_size)
    {
        start_sector -= nftl_blk->nftl_logic_size;
        nftl_blk = nftl_blk->nftl_blk_next;
        if(nftl_blk == NULL)
        {
            printk("[NE]parameter error %d,%d !\n",start_sector,len);
            return 1;
        }
    }

    mutex_lock(nftl_blk->blk_lock);
    ret = nftl_blk->write_data(nftl_blk,start_sector,len,buf);
    mutex_unlock(nftl_blk->blk_lock);

    return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nftl_test_flush_write_cache(void)
{
    struct _nftl_blk *nftl_blk;
    nftl_blk = nftl_blk_head.nftl_blk_next;

    while(nftl_blk != NULL)
    {
        mutex_lock(nftl_blk->blk_lock);
        nftl_blk->flush_write_cache(nftl_blk,0xffff);
        mutex_unlock(nftl_blk->blk_lock);
        nftl_blk = nftl_blk->nftl_blk_next;
    }
	return 0;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void obj_test_release(struct kobject *kobject)
{
    printk("release");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t nand_test_show(struct kobject *kobject,struct attribute *attr, char *buf)
{
    ssize_t count = 0;
    struct nand_kobject* nand_kobj;

    nand_kobj = (struct nand_kobject*)kobject;

    print_nftl_zone(nand_kobj->nftl_blk->nftl_zone);

    return count;
}

/*****************************************************************************
*Name         :
*Description  :receive testcase num from echo command
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t nand_test_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count)
{
    int             ret;
    int             argnum = 0;
    char            cmd[32] = {0};
    unsigned int    param0 = 0;
    unsigned int    param1 = 0;
    unsigned int    param2 = 0;

    struct nand_kobject* nand_kobj;
    nand_kobj = (struct nand_kobject*)kobject;

    argnum = sscanf(buf, "%s %u %u %u ", cmd, &param0, &param1, &param2);
    printk("argnum=%i, cmd=%s, param0=%u, param1=%u, param2=%u\n", argnum, cmd, param0, param1, param2);

    if (-1 == argnum)
    {
        printk("cmd format err!");
        goto NAND_TEST_STORE_EXIT;
    }

    if(strcmp(cmd,"help") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  help \n");
    }
    else if(strcmp(cmd,"flush") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  flush \n");
        mutex_lock(nand_kobj->nftl_blk->blk_lock);
        ret = nand_kobj->nftl_blk->flush_write_cache(nand_kobj->nftl_blk,param0);
        mutex_unlock(nand_kobj->nftl_blk->blk_lock);
        goto NAND_TEST_STORE_EXIT;
    }
    else if(strcmp(cmd,"gcall") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  gcall \n");
        mutex_lock(nand_kobj->nftl_blk->blk_lock);
        ret = gc_all(nand_kobj->nftl_blk->nftl_zone);
        mutex_unlock(nand_kobj->nftl_blk->blk_lock);
        goto NAND_TEST_STORE_EXIT;
    }
    else if(strcmp(cmd,"gcone") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  gcone \n");
        mutex_lock(nand_kobj->nftl_blk->blk_lock);
        ret = gc_one(nand_kobj->nftl_blk->nftl_zone);
        mutex_unlock(nand_kobj->nftl_blk->blk_lock);
        goto NAND_TEST_STORE_EXIT;
    }
    else if(strcmp(cmd,"test") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  test \n");
        mutex_lock(nand_kobj->nftl_blk->blk_lock);
        ret = nftl_set_zone_test((void*)nand_kobj->nftl_blk->nftl_zone,param0);
        mutex_unlock(nand_kobj->nftl_blk->blk_lock);
        goto NAND_TEST_STORE_EXIT;
    }
    else if(strcmp(cmd,"showall") == 0)
    {
        printk("nand debug cmd:\n");
        printk("  show all \n");
        print_free_list(nand_kobj->nftl_blk->nftl_zone);
        print_block_invalid_list(nand_kobj->nftl_blk->nftl_zone);
        print_nftl_zone(nand_kobj->nftl_blk->nftl_zone);
        goto NAND_TEST_STORE_EXIT;
    }
    else
    {
        printk("err, nand debug undefined cmd: %s\n", cmd);
    }

NAND_TEST_STORE_EXIT:
    return count;
}
