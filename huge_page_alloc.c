#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "hugepage_dev"
#define CLASS_NAME  "huge"

#define IOCTL_GET_PHYS_ADDR _IOR('h', 1, unsigned long)

/* 예약된 물리 메모리 영역 (boot 시 memmap 등으로 예약한 영역이어야 함) */
#define RESERVED_MEM_PHYS 0x857000000UL

/* HUGE_PAGE_ORDER: 여기서는 10이면 4MB (2^(12+10)=2^22) */
#define HUGE_PAGE_ORDER 10

static int major;
static struct class *hugepage_class = NULL;
static struct device *hugepage_device = NULL;
static struct cdev hugepage_cdev;

/* ioremap()의 반환값은 void __iomem* 타입으로 처리합니다 */
static void __iomem *huge_buf = NULL;
static unsigned long huge_buf_size;

/* 예약된 영역의 물리 주소는 그대로 저장 */
static unsigned long reserved_phys_addr = RESERVED_MEM_PHYS;

static long hugepage_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned long phys_addr;
    switch (cmd) {
    case IOCTL_GET_PHYS_ADDR:
        /* ioremap()으로 매핑한 경우, 실제 물리 주소는 예약한 주소입니다. */
        phys_addr = reserved_phys_addr;
        if (copy_to_user((unsigned long __user *)arg, &phys_addr, sizeof(phys_addr)))
            return -EFAULT;
        pr_info("hugepage: returned phys_addr = 0x%lx\n", phys_addr);
        return 0;
    default:
        return -ENOTTY;
    }
}

static int hugepage_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int hugepage_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int hugepage_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long pfn = reserved_phys_addr >> PAGE_SHIFT;
    size_t size = vma->vm_end - vma->vm_start;

    if (size > huge_buf_size)
        return -EINVAL;

    if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

static const struct file_operations hugepage_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = hugepage_ioctl,
    .open           = hugepage_open,
    .release        = hugepage_release,
    .mmap           = hugepage_mmap,
};

static int __init hugepage_init(void)
{
    int ret;
    dev_t dev_no;
    pr_info("hugepage_module: init\n");

    /* 할당 크기 계산: 여기서는 HUGE_PAGE_ORDER 10이면 4MB */
    huge_buf_size = PAGE_SIZE << HUGE_PAGE_ORDER;

    /* 예약된 물리 메모리 영역을 ioremap()으로 매핑 */
    huge_buf = ioremap(reserved_phys_addr, huge_buf_size);
    if (!huge_buf) {
        pr_err("hugepage_module: failed to ioremap reserved page\n");
        return -ENOMEM;
    }
    pr_info("hugepage_module: mapped huge_buf at vaddr=%p, size=%lu bytes\n", huge_buf, huge_buf_size);
    pr_info("hugepage_module: reserved phys_addr = 0x%lx\n", reserved_phys_addr);

    /* 문자 디바이스 등록 */
    ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("hugepage_module: alloc_chrdev_region failed\n");
        goto err_alloc;
    }
    major = MAJOR(dev_no);

    cdev_init(&hugepage_cdev, &hugepage_fops);
    hugepage_cdev.owner = THIS_MODULE;
    ret = cdev_add(&hugepage_cdev, dev_no, 1);
    if (ret < 0) {
        pr_err("hugepage_module: cdev_add failed\n");
        goto err_cdev;
    }
    hugepage_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hugepage_class)) {
        pr_err("hugepage_module: class_create failed\n");
        ret = PTR_ERR(hugepage_class);
        goto err_class;
    }
    hugepage_device = device_create(hugepage_class, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(hugepage_device)) {
        pr_err("hugepage_module: device_create failed\n");
        ret = PTR_ERR(hugepage_device);
        goto err_device;
    }

    pr_info("hugepage_module: device /dev/%s created with major %d\n", DEVICE_NAME, major);
    return 0;

err_device:
    class_destroy(hugepage_class);
err_class:
    cdev_del(&hugepage_cdev);
err_cdev:
    unregister_chrdev_region(dev_no, 1);
err_alloc:
    iounmap(huge_buf);
    return ret;
}

static void __exit hugepage_exit(void)
{
    dev_t dev_no = MKDEV(major, 0);
    device_destroy(hugepage_class, dev_no);
    class_destroy(hugepage_class);
    cdev_del(&hugepage_cdev);
    unregister_chrdev_region(dev_no, 1);
    /* ioremap()로 매핑한 영역은 free_pages()가 아닌 iounmap()으로 해제 */
    iounmap(huge_buf);
    pr_info("hugepage_module: exit\n");
}

module_init(hugepage_init);
module_exit(hugepage_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author Name");
MODULE_DESCRIPTION("Kernel module to map reserved physical memory and provide mmap/ioctl interface");

