# Makefile for hugepage_sysfs kernel module

# 컴파일할 대상 소스 파일 (객체 파일)
obj-m += huge_page_alloc.o

# 현재 커널 빌드 디렉터리
KDIR := /lib/modules/$(shell uname -r)/build
# 현재 소스 파일 위치
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
