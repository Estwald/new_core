/* 
    (c) 2013 Estwald <www.elotrolado.net>

    "install core" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "install core" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "install core".  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>


#include <sys/process.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>
#include <sys/file.h>
#define FS_S_IFMT 0170000


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


static int sys_reboot()
{
    unlink_secure("/dev_hdd0/tmp/turnoff");

    lv2syscall4(379,0x1200,0,0,0);
    return_to_user_prog(int);
}


int sys_fs_mount(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt) 
{
    lv2syscall8(837, (u64) deviceName, (u64) deviceFileSystem, (u64) devicePath, 0, (u64) writeProt, 0, 0, 0 );
    return_to_user_prog(int);
}

int sys_fs_umount(char const* devicePath) 
{
    lv2syscall3(838,  (u64) devicePath, 0, 0 );
    return_to_user_prog(int);
}

int sys_set_leds(u64 color, u64 state) 
{
    lv2syscall2(386,  (u64) color, (u64) state);
    return_to_user_prog(int);
}




char * LoadFile(char *path, int *file_size)
{
    FILE *fp;
    char *mem = NULL;
    
    *file_size = 0;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    fp = fopen(path, "rb");

	if (fp != NULL) {
        
        fseek(fp, 0, SEEK_END);
		
        *file_size = ftell(fp);
        
        mem = malloc(*file_size);

		if(!mem) {fclose(fp);return NULL;}
        
        fseek(fp, 0, SEEK_SET);

		if(*file_size != fread((void *) mem, 1, *file_size, fp)) {
            fclose(fp); *file_size = 0; free(mem); return NULL;
        }

		fclose(fp);

    }

    return mem;
}

int SaveFile(char *path, char *mem, int file_size)
{
    FILE *fp;
   
    fp = fopen(path, "wb");

	if (fp != NULL) {
       
		if(file_size != fwrite((void *) mem, 1, file_size, fp)) {
            fclose(fp); return -1;
        }

		fclose(fp);

    } else return -1;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    return 0;
}


char self_path[0x420];

s32 main(s32 argc, const char* argv[])
{


    char path[0x420];   

    if(argc>0 && argv) {
    
        if(!strncmp(argv[0], "/dev_hdd0/game/", 15)) {
            int n;

            strcpy(self_path, argv[0]);

            n= 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

            if(self_path[n] == '/') {
                self_path[n] = 0;
            }
        }
    }

    if(sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_rewrite", 0)==0);

    sprintf(path, "%s/USRDIR/new_core.self", self_path);
    
    int file_size;
    sysFSStat s;

    if(filestat(path, &s)==0) {

        char * mem = LoadFile(path, &file_size);
        if(mem && file_size > 0 && s.st_size == file_size) {
            
            if(SaveFile("/dev_rewrite/sys/internal/sys_init_osd_temp.self", mem, file_size)!=0) {
                unlink_secure("/dev_rewrite/sys/internal/sys_init_osd_temp.self");
                exit(0);
            }

            if(sysLv2FsRename("/dev_rewrite/sys/internal/sys_init_osd.self", "/dev_rewrite/sys/internal/sys_init_osd_old.self")!=0) {
                unlink_secure("/dev_rewrite/sys/internal/sys_init_osd.self");
            }

            if(sysLv2FsRename("/dev_rewrite/sys/internal/sys_init_osd_temp.self", "/dev_rewrite/sys/internal/sys_init_osd.self")!=0) {
                sysLv2FsRename("/dev_rewrite/sys/internal/sys_init_osd_old.self", "/dev_rewrite/sys/internal/sys_init_osd.self"); // paranoid code!
            }

            sys_reboot();

        }
    }


	return 0;
}
