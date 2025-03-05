**Add a Reserved Memory Region to GRUB**

Before loading this kernel module, reserve a memory region by adding the following parameter to your `/etc/default/grub`:

```
GRUB_CMDLINE_LINUX="memmap=16M\$0x857000000"
```

After making this change, remember to **update GRUB** and **reboot** so the new parameter takes effect.
