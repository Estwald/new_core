/* 
    (c) 2013 Estwald <www.elotrolado.net>
    (c) 2013 MiralaTijera <www.elotrolado.net> (Original Core)

    "New Core" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "New Core" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "New Core". If not, see <http://www.gnu.org/licenses/>.

*/

/*

NOTE: This program is based partially in the MiralaTijera Core

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ppu-lv2.h>

#include <sys/memory.h>
#include <sys/process.h>
#include <sys/systime.h>
#include <sys/thread.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <lv2/sysfs.h>

#define FS_S_IFMT 0170000

int dev_rw_mounted = 0;
sysFSStat stat1;
int verbose = 0;
int from_reboot = 0;

u64 lv2peek(u64 addr) 
{ 
    lv2syscall1(6, (u64) addr);
    return_to_user_prog(u64);

}

u64 lv2poke(u64 addr, u64 value) 
{ 
    lv2syscall2(7, (u64) addr, (u64) value); 
    return_to_user_prog(u64);
}

u32 lv2peek32(u64 addr) {
    u32 ret = (u32) (lv2peek(addr) >> 32ULL);
    return ret;
}

u64 lv2poke32(u64 addr, u32 value) 
{ 
    return lv2poke(addr, (((u64) value) <<32) | (lv2peek(addr) & 0xffffffffULL));
}

extern int _sys_process_atexitspawn(u32 a, const char *file, u32 c, u32 d, u32 e, u32 f);

int launchself(const char*file)
{
	return _sys_process_atexitspawn(0, file, 0, 0, 0x3d9, 0x20);
    
}

int launchselfback(const char*file)
{
	return _sys_process_atexitspawn(0, file, 0, 0, 0x7d0, 0x20);
    
}

int sys_fs_mount_ext(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt, u32* buffer, u32 count) 
{
    lv2syscall8(837, (u64) deviceName, (u64) deviceFileSystem, (u64) devicePath, 0ULL, (u64) writeProt, 0ULL, (u64) buffer, (u64) count);
    return_to_user_prog(int);
}

int sys_fs_umount(char const* devicePath) 
{
    lv2syscall3(838,  (u64) devicePath, 0, 0 );
    return_to_user_prog(int);
}

int filestat(const char *path, sysFSStat *stat)
{
    int ret = sysLv2FsStat(path, stat);

    if(ret == 0 && S_ISDIR(stat->st_mode)) return -1;
    
    return ret;
}

int unlink_secure(void *path)
{
    sysFSStat s;
    if(filestat(path, &s)>=0) {
        sysLv2FsChmod(path, FS_S_IFMT | 0777);
        return sysLv2FsUnlink(path);
    }
    return -1;
}

int sys_shutdown()
{   
    unlink_secure("/dev_hdd0/tmp/turnoff");
    
    lv2syscall4(379,0x1100,0,0,0);
    return_to_user_prog(int);
}

static int sys_reboot()
{
    unlink_secure("/dev_hdd0/tmp/turnoff");

    lv2syscall4(379,0x1200,0,0,0);
    return_to_user_prog(int);
}

int try_mount_usb0()
{   
    int ret;
    u32 usb_buf[64]; // 0xA is not sufficient!
    char dev[64];
    int n;
    strncpy(dev, "CELL_FS_IOS:USB_MASS_STORAGE000", 64);
     
    // try mount dev_usb000
    for(n = 0; n < 5; n++) {
        memset(usb_buf, 0, sizeof(usb_buf));
  
        ret = sys_fs_mount_ext(dev, "CELL_FS_FAT", "/dev_usb000", 0, usb_buf, 0);
        if(ret == (int) 0x8001002B) sysSleep(3);
        else break;
        
    }
  
    return ret;
}

int mount_flash()
{
    if(dev_rw_mounted || (!dev_rw_mounted && sys_fs_mount_ext("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_rewrite", 0, NULL, 0)==0)) {

        dev_rw_mounted = 1;
    }

    return dev_rw_mounted;

}

char buffer[65536];

int file_copy(const char *src, const char *dst)
{
    sysFSStat stat1;
    int fd, fd2;
    int ret;
    u64 temp = 0;
    u64 readed = 0;

    if(filestat(src, &stat1)!=0 || stat1.st_size == 0) return -1;

    if(!sysLv2FsOpen(src, SYS_O_RDONLY, &fd, 0, NULL, 0)) {
        if(!sysLv2FsOpen(dst, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0)) {
            sysLv2FsChmod(dst, FS_S_IFMT | 0777);

            while(stat1.st_size != 0ULL) {
                readed = stat1.st_size;
                if(readed > 65536) readed = 65536;
                temp = 0;
                ret = sysLv2FsRead(fd, buffer, readed, &temp);
                if(ret < 0 || readed != temp) break;
                ret = sysLv2FsWrite(fd2, buffer, readed, &temp);
                if(ret < 0 || readed != temp) break;

                stat1.st_size -= readed;
            }

            sysLv2FsClose(fd);
            sysLv2FsClose(fd2);

            if(stat1.st_size) return -4;
        } else {
            sysLv2FsClose(fd);
            return -3;
        }
    } else return -2;

    return 0;
}


int fd_log = -1;


void CloseLog()
{
    if(fd_log >= 0) sysLv2FsClose(fd_log);

    fd_log = -1;
}

int WriteLog(char *str)
{
    u64 temp = 0;
    u64 towrite;

    if(fd_log < 0 || !str) return -1;

    towrite = strlen(str);

    if(towrite == 0) return 0;

    if(sysLv2FsWrite(fd_log, str, towrite, &temp) || temp!=towrite) return -2;

    return 0;

}

int Open_Log(char *file)
{
    if(fd_log >= 0) return -666;

    if(!sysLv2FsOpen(file, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd_log, 0777, NULL, 0)) {
        sysLv2FsChmod(file, FS_S_IFMT | 0777);

        if(WriteLog("New Core v1.0\r\n")!=0) {CloseLog();return -2;}
        if(WriteLog("(c) 2013, Estwald\r\n\r\n")!=0) {CloseLog();return -2;}
        return 0;
    }
    
    fd_log = -1;

    return -1;
    
}


void actions()
{
    int ret;
    int fd;

    if(filestat("/dev_usb000/core_flags/verbose", &stat1)==0) {
        verbose = 1; Open_Log("/dev_usb000/core_log.txt");

        if(from_reboot) WriteLog("Working from Reboot\r\n\r\n");
    }

    if(filestat("/dev_usb000/core_flags/boot_on", &stat1)==0) {
        if(mount_flash()) {

            if(!sysLv2FsOpen("/dev_rewrite/force_from_reboot", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0)) {
                sysLv2FsChmod("/dev_rewrite/force_from_reboot", FS_S_IFMT | 0777);
                sysLv2FsClose(fd);
                WriteLog("'force_from_reboot' flag created in /dev_flash\r\n");

                if(sysLv2FsRename("/dev_usb000/core_flags/boot_on", "/dev_usb000/core_flags/_boot_on") != 0) {
                    if(unlink_secure("/dev_usb000/core_flags/boot_on") !=0) {WriteLog("Error! 'boot_on' flag cannot be removed\r\n");}
                    else WriteLog("'boot_on' flag removed\r\n");
                } else WriteLog("'boot_on' flag removed\r\n");


            } else WriteLog("fail to create 'force_from_reboot' in /dev_flash\r\n");
        
        }
        
    }

    if(filestat("/dev_usb000/core_flags/boot_off", &stat1)==0) {
        if(mount_flash()) {

            ret = unlink_secure("/dev_rewrite/force_from_reboot"); if(ret == -1) ret = 0;

            if(ret == 0) {
                
                WriteLog("'force_from_reboot' flag removed from /dev_flash\r\n");

                if(sysLv2FsRename("/dev_usb000/core_flags/boot_off", "/dev_usb000/core_flags/_boot_off") != 0) {
                    if(unlink_secure("/dev_usb000/core_flags/boot_off") !=0 ) {WriteLog("Error! 'boot_off' flag cannot be removed\r\n");}
                    else WriteLog("'boot_off' flag removed\r\n");
                } else WriteLog("'boot_off' flag removed\r\n");


            } else WriteLog("fail to remove 'force_from_reboot' from /dev_flash\r\n");
        
        }
        
    }

    if(filestat("/dev_usb000/core_flags/install", &stat1)==0 || filestat("/dev_usb000/core_flags/install2", &stat1)==0) {
       int with_errors = 0;
       int with_errors2 = 0;
       
       if(mount_flash()) {

            if(!verbose) {
                Open_Log("/dev_usb000/core_log.txt");
                if(from_reboot) WriteLog("Working from Reboot\r\n\r\n");
            }
            
            if(filestat("/dev_usb000/core_flags/install", &stat1)==0) {

                ret = 0;

                WriteLog("Using 'install' flag:\r\n\r\n");

                if(sysLv2FsRename("/dev_usb000/core_flags/install", "/dev_usb000/core_flags/_install") != 0) {
                    if(unlink_secure("/dev_usb000/core_flags/install")!=0) {ret = -1; WriteLog("Error! 'install' flag cannot be removed\r\n");}
                    else WriteLog("'install' flag removed\r\n");
                } else WriteLog("'install' flag removed\r\n");

                    
                if(ret == 0 && !sysLv2FsOpen("/dev_usb000/core_flags/install2", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0)) {
                    sysLv2FsChmod("/dev_usb000/core_flags/install2", FS_S_IFMT | 0777);
                    sysLv2FsClose(fd);
                    WriteLog("'install2' flag created\r\n");

                } else ret = -1;


                if(ret == 0 && filestat("/dev_flash/force_from_reboot", &stat1)==0) {
                    WriteLog("Rebooting to install\r\n");
                    CloseLog();
                    sys_reboot();
                } else if(ret == 0 && !sysLv2FsOpen("/dev_rewrite/from_reboot", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0)) {
                    sysLv2FsClose(fd);
                    sysLv2FsChmod("/dev_rewrite/from_reboot", FS_S_IFMT | 0777);   
                    WriteLog("Rebooting to install\r\n");
                    CloseLog();
                    sys_reboot();
                }

                if(ret != 0) goto skip_install2;
            }

            
            WriteLog("Using 'install2' flag:\r\n\r\n");

            with_errors2 = 0;

            ret = file_copy("/dev_usb000/core_install/sm.self", "/dev_rewrite/sm_temp.self");
            // ret  0 = OK
            // ret -1 = file not found

            if(ret == 0) WriteLog("sm.self copied as sm_temp.self\r\n");
            else if(ret != -1) {WriteLog("Error copying sm.self\r\n"); with_errors = 1; with_errors2 = 1;}
            
            if(ret != 0) unlink_secure("/dev_rewrite/sm_temp.self");
            else {
                unlink_secure("/dev_rewrite/sm.self");
                if(sysLv2FsRename("/dev_rewrite/sm_temp.self", "/dev_rewrite/sm.self") != 0){
                    with_errors = 1; with_errors2 = 1;
                    WriteLog("Error in sysLv2FsRename() function\r\n");
                    unlink_secure("/dev_rewrite/sm_temp.self");
                } else {
                    WriteLog("sm.self installed!\r\n");
                }
            }

            if(!with_errors2 && ret != -1) {

                unlink_secure("/dev_usb000/core_install/_sm.self");
                if(sysLv2FsRename("/dev_usb000/core_install/sm.self", "/dev_usb000/core_install/_sm.self")!=0) {

                    WriteLog("Warning! I cannot rename sm.self to _sm.self from /dev_usb000");

                }
            }

            if(ret != -1) WriteLog("\r\n");

            with_errors2 = 0;

            // copy the core to one temporal location
            ret = file_copy("/dev_usb000/core_install/new_core.self", "/dev_rewrite/sys/internal/sys_init_osd_temp.self");
            // ret  0 = OK
            // ret -1 = file not found


            if(ret == 0) WriteLog("new_core.self copied as sys_init_osd_temp.self\r\n");
            else if(ret!=-1) {WriteLog("Error copying new_core.self\r\n"); with_errors = 1; with_errors2 = 1;}
            
            if(ret == 0) {
                unlink_secure("/dev_rewrite/sys/internal/sys_init_osd_old.self");
                if(sysLv2FsRename("/dev_rewrite/sys/internal/sys_init_osd.self", "/dev_rewrite/sys/internal/sys_init_osd_old.self")==0) {
                    if(sysLv2FsRename("/dev_rewrite/sys/internal/sys_init_osd_temp.self", "/dev_rewrite/sys/internal/sys_init_osd.self")!=0) {
                        with_errors = 1; with_errors2 = 1;
                        WriteLog("Error in sysLv2FsRename() function (sys_init_osd_temp.self to sys_init_osd.self)\r\n");
                    } else WriteLog("new_core.self installed!\r\n");
                } else {
                    with_errors = 1; with_errors2 = 1;
                    WriteLog("Error in sysLv2FsRename() function (sys_init_osd.self to sys_init_osd_old.self)\r\n");
                    unlink_secure("/dev_rewrite/sys/internal/sys_init_osd_old.self");
                }
            } else unlink_secure("/dev_rewrite/sys/internal/sys_init_osd_temp.self");
            
            if(!with_errors2 && ret != -1) {

                unlink_secure("/dev_usb000/core_install/_new_core.self");
                if(sysLv2FsRename("/dev_usb000/core_install/new_core.self", "/dev_usb000/core_install/_new_core.self")!=0) {

                    WriteLog("Warning! I cannot rename new_core.self to _new_core.self from /dev_usb000");

                }
            }
            
            if(ret != -1) WriteLog("\r\n");

            unlink_secure("/dev_usb000/core_flags/install"); // here never can exist 'install' flag
            unlink_secure("/dev_rewrite/from_reboot"); // here never can exist 'from_reboot' flag
            
            ret = 0;

            if(sysLv2FsRename("/dev_usb000/core_flags/install2", "/dev_usb000/core_flags/_install2") != 0) {
                ret = unlink_secure("/dev_usb000/core_flags/install2"); if(ret == -1) ret = 0;
                if(ret != 0) {WriteLog("Error! 'install' flag cannot be removed\r\n"); with_errors = 1;}
                else WriteLog("'install2' flag removed\r\n");

            } else WriteLog("'install2' flag removed\r\n");

            WriteLog("\r\n");

            unlink_secure("/dev_usb000/install_log.txt");

            if(!with_errors) {
                
                WriteLog("Rebooting from 'install2'\r\n");
                CloseLog(); 
                if(sysLv2FsRename("/dev_usb000/core_log.txt", "/dev_usb000/install_log.txt") == 0) sys_reboot();
                else sys_shutdown();
            }

       skip_install2:

            if(!verbose) CloseLog();
       }
    }
     
   
    if(filestat("/dev_usb000/core_flags/removesm", &stat1) == 0) {
       
        if(mount_flash()) {

            if(verbose) WriteLog("Using 'removesm' flag:\r\n\r\n");
            unlink_secure("/dev_rewrite/sm.self");
            
        }
    }


}

void launch_sm_self()
{
    int ret;
    sysFSStat stat1;

    ret = filestat("/dev_usb000/core_flags/ignoresm", &stat1);
    if(ret == 0 && verbose) WriteLog("Using 'ignoresm' flag:\r\n\r\n");

    if(ret!=0) {
        
        if(filestat("/dev_usb000/sm_external.self", &stat1) == 0 && stat1.st_size > 0 && launchselfback("/dev_usb000/sm_external.self")==0) {
            ret = 0;
            if(verbose) WriteLog("'sm_external.self' working from /dev_usb000\r\n\r\n");
        }

        
        if(ret != 0 && filestat("/dev_flash/sm.self", &stat1) == 0 && stat1.st_size > 0 && launchselfback("/dev_flash/sm.self")==0) {
            ret = 0;
            if(verbose) WriteLog("'sm.self' working from /dev_flash\r\n\r\n");
        }
        
    }

}


s32 main(s32 argc, const char* argv[]) 
{
    
    sysFSStat stat1;
    int n;
    

    if(filestat("/dev_flash/from_reboot", &stat1)==0 || filestat("/dev_flash/force_from_reboot", &stat1)==0) {
        from_reboot = 1;
        try_mount_usb0();

        if(mount_flash()) {

            unlink_secure("/dev_rewrite/from_reboot");
        }

        actions();

        if(dev_rw_mounted) {
            sys_fs_umount("/dev_rewrite");
            dev_rw_mounted = 0;
        }

    }

    if(launchself("/dev_flash/sys/internal/sys_init_osd_orig.self")!=0) // launch VSH at first to avoid problems with the PAD
        if(launchself("/dev_flash/vsh/module/vsh.self")!=0) {
            // emergency! try to launch VSH.SELF from USB0
            try_mount_usb0();

            Open_Log("/dev_usb000/emergency_log.txt");

            WriteLog("sys_init_osd_orig.self and vsh.self not found or corrupt!\r\n\r\n");
            if(sys_fs_mount_ext("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_rewrite", 0, NULL, 0)==0) {
                WriteLog("/dev_rewrite mounted\r\n\r\n");
            }
            WriteLog("Trying to launch /dev_usb000/emergency.self ...!\r\n\r\n");
            
            if(launchself("/dev_usb000/emergency.self")!=0) {
                WriteLog("Failed!");
                CloseLog();
                sys_shutdown();
            }
            WriteLog("Success!");
            CloseLog();
            return 0;
    }

    // wait to VSH.SELF mount /dev_usb000 if is connected

    if(!from_reboot) {
        for(n = 0; n < 5; n++) {
            sysSleep(3);
            if(!filestat("/dev_usb000", &stat1)) break;
        }

        actions();
    
        if(dev_rw_mounted) {
            sys_fs_umount("/dev_rewrite");
            dev_rw_mounted = 0;  
        } 
    }

    launch_sm_self();

    if(verbose) WriteLog("Bye!");
    CloseLog();

    sysSleep(1);
    
	return 0;
}

