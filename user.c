#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define IOCTL_GET_PHYS_ADDR _IOR('h', 1, unsigned long)

int main(void)
{
    int fd = open("/dev/hugepage_dev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    unsigned long phys_addr;
    if (ioctl(fd, IOCTL_GET_PHYS_ADDR, &phys_addr) < 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }
    printf("Huge page physical address: 0x%lx\n", phys_addr);
    close(fd);
    return 0;
}

