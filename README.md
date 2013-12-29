new_core v1.0
=============

New Core to launch sm.self

Based partially in the work of MiralaTijera, this utility replace sys_init_osd.self and launch vsh.self directly (or original sys_init_osd.self as sys_init_osd_orig.self).
i
It have two working ways:

- With "boot_on" or "install" flags it mount /dev_usb000 device, operates and launch vsh.self (or sys_init_osd_orig.sys). This is the secure mode, but it causes the ps3 pad desynchronization (same case of MiralaTijera Core)

- Without "boot_on" or with "install2" flags it operates AFTER of vsh.self load (/dev_usb000 is mounted by vsh.self) working in second plane


In theory, this core can work with alls CFW, but is is only tested under 4.46 and 4.53 CFW working (without plugins or other application that may cause incompatibility): sm.self is only ready for 3.41, 3.55, 4.21. 4.30. 4.31, 4.40, 4.46, 4.50 and 4.53 CEX versions. 


flags (from /dev_usb000/core_flags):
====================================
boot_on : install in /dev_flash "force_from_reboot" flag to operates BEFORE of vsh.self load. Without reboot.

boot_off : delete from /dev_flash "force_from_reboot" flag to operates AFTER of vsh.self load. Without reboot.

install : install in /dev_flash "from_reboot" flag temporally, try to install in /dev_usb000/core_flag/install2 and reboot to install files BEFORE vsh.self load (secure mode)

install2 : enables the files installation from /dev_usb000/core_install. Currently is supported sm.self and new_core.self (see install_log.txt for details)

verbose : enables full log from the core (core_log.txt). Usually log is disabled except when you install files.

removesm : deletes sm.self from /dev_flash

ignoresm : ignore the load of sm.self (test flag)

NOTES:

- Some flags are disabled prefixing the character underline . The same thing when you install sm.self or new_core.self.

- Some functions reboot the system from the XMB: don´t worry XD
