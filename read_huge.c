#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define IOCTL_GET_PHYS_ADDR _IOR('h', 1, unsigned long)
#define HUGEPAGE_SIZE (2 * 1024 * 1024)  // 2MB huge page

int main(void) {
    int fd;
    void *huge_base;
    uint64_t phys_addr;

    // 1. /dev/hugepage_dev 열기
    fd = open("/dev/hugepage_dev", O_RDWR);
    if (fd < 0) {
        perror("open /dev/hugepage_dev");
        exit(EXIT_FAILURE);
    }

    // 2. huge page의 물리 주소 얻기 (ioctl)
    if (ioctl(fd, IOCTL_GET_PHYS_ADDR, &phys_addr) < 0) {
        perror("ioctl IOCTL_GET_PHYS_ADDR");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("Huge page physical address: 0x%lx\n", phys_addr);

    // 3. huge page 메모리 매핑 (mmap)
    huge_base = mmap(NULL, HUGEPAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (huge_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    printf("Huge page mapped at virtual address: %p\n", huge_base);

    // 4. huge page의 첫 4바이트 읽기
    uint64_t first_dword = *((volatile uint64_t*)huge_base + 0x5000);
    uint64_t s_dword = *((volatile uint64_t*)huge_base + 0x5000 + 0x4);
    uint64_t t_dword = *((volatile uint64_t*)huge_base + 0x5000 + 0x8);
    uint64_t f_dword = *((volatile uint64_t*)huge_base + 0x5000 + 0x12);
    printf("The first 4 bytes of the huge page are: 0x%016lX\n", first_dword);
    printf("The 2nd  4 bytes of the huge page are: 0x%016lX\n", s_dword);
    printf("The 3rd 4 bytes of the huge page are: 0x%016lX\n", t_dword );
    printf("The 4th 4 bytes of the huge page are: 0x%016lX\n", f_dword );

    // 5. 자원 해제
    munmap(huge_base, HUGEPAGE_SIZE);
    close(fd);

    return 0;
}

