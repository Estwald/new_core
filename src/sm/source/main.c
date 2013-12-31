/* 
    (c) 2013 Estwald <www.elotrolado.net>

    "System Manager" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "System Manager" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "System Manager". If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>
#include <ppu-lv2.h>

#include <sys/memory.h>
#include <sys/process.h>
#include <sys/systime.h>
#include <sys/thread.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <lv2/sysfs.h>

#define FS_S_IFMT 0170000

#define LV2_SM_CMD_ADDR 0x8000000000000450ULL

#define REBUG 1

#ifdef REBUG
static u64 lv1poke(u64 addr, u64 value) 
{ 
    lv2syscall2(9, (u64) addr, (u64) value); 
    return_to_user_prog(u64);
}
#endif


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

int sys_game_get_temperature(int sel, u32 *temperature) 
{
    u32 temp;
  
    lv2syscall2(383, (u64) sel, (u64) &temp); 
    *temperature = (temp >> 24);
    return_to_user_prog(int);
}

int sys_sm_set_fan_policy(u8 arg0, u8 arg1, u8 arg2)
{
    lv2syscall3(389, (u64) arg0, (u64) arg1, (u64) arg2); 
    return_to_user_prog(int);
}

int sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
    lv2syscall5(409, (u64) id, (u64) st, (u64) mode, (u64) speed, (u64) unknown); 
    return_to_user_prog(int);
}


int sys_set_leds(u64 color, u64 state) 
{
    lv2syscall2(386,  (u64) color, (u64) state);
    return_to_user_prog(int);
}

u32 cfw = 0x440C;
u64 syscall_base = 0x800000000035E260ULL;
static u64 PAYLOAD_BASE = 0x8000000000000f70ULL;

#include "payload_bin.h"

u64 addr[1024/8];

void patchs_lv2(){

    // CFW identify

    int n, noload = 0;
    u64 uval;

    uval= lv2peek(0x8000000000003000ULL);

    cfw = 0;

    if(uval == 0x800000000033E720ULL) cfw = 0x341C; else
    if(uval == 0x8000000000330540ULL) cfw = 0x355C; else
    if(uval == 0x8000000000346390ULL) cfw = 0x421C; else
    if(uval == 0x8000000000348200ULL) cfw = 0x430C; else
    if(uval == 0x8000000000348210ULL) cfw = 0x431C; else
    if(uval == 0x80000000003487D0ULL) cfw = 0x440C; else
    if(uval == 0x8000000000348DF0ULL) cfw = 0x446C; else
    if(uval == 0x800000000034B160ULL) cfw = 0x450C; else
    if(uval == 0x800000000034B2E0ULL) cfw = 0x453C;

    if(cfw == 0x341C) {
        
        syscall_base = 0x80000000002EB128ULL;

    } else if(cfw == 0x355C) {
        
        syscall_base = 0x8000000000346570ULL;

    } else if(cfw == 0x421C) {
        
        syscall_base = 0x800000000035BCA8ULL;

        #ifdef REBUG
        // Remove LV2 memory protection using LV1_POKE (syscall 9). Maybe unnecessary
 
        lv1poke(0x370A28, 0x0000000000000001ULL);
        lv1poke(0x370A30, 0xe0d251b556c59f05ULL);
        lv1poke(0x370A38, 0xc232fcad552c80d7ULL);
        lv1poke(0x370A40, 0x65140cd200000000ULL);
        #endif

    } else if(cfw == 0x430C) {
        
        syscall_base = 0x800000000035DBE0ULL;

        #ifdef REBUG
        // Remove LV2 memory protection using LV1_POKE (syscall 9). Maybe unnecessary
 
        lv1poke(0x370AA8 + 0, 0x0000000000000001ULL);
        lv1poke(0x370AA8 + 8, 0xe0d251b556c59f05ULL);
        lv1poke(0x370AA8 + 16, 0xc232fcad552c80d7ULL);
        lv1poke(0x370AA8 + 24, 0x65140cd200000000ULL);

        #endif

    } else if(cfw == 0x431C) {
        
        syscall_base = 0x800000000035DBE0ULL;

    } else if(cfw == 0x440C) {
    
        syscall_base = 0x800000000035E260ULL;
    
    } else if(cfw == 0x446C) {
    
        syscall_base = 0x800000000035E860ULL;

        #ifdef REBUG
        // Remove LV2 memory protection using LV1_POKE (syscall 9). Maybe unnecessary
 
        lv1poke(0x370AA8, 0x0000000000000001ULL);
        lv1poke(0x370AA8 + 8, 0xE0D251B556C59F05ULL);
        lv1poke(0x370AA8 + 16, 0xC232FCAD552C80D7ULL);
        lv1poke(0x370AA8 + 24, 0x65140CD200000000ULL);
        #endif
    
    } else if(cfw == 0x450C) {
    
        syscall_base = 0x800000000035F0D0ULL;
    
    } else if(cfw == 0x453C) {
    
        syscall_base = 0x800000000035F300ULL;
    
    } else exit(0);

    if(lv2peek(PAYLOAD_BASE)) {noload=1;goto skip_the_load;}
    memcpy((char *) addr, (char *) payload_bin, payload_bin_size);

    addr[1] = syscall_base;

    addr[2] += PAYLOAD_BASE;
    addr[3] = lv2peek(syscall_base + (u64) (379 * 8));

    skip_the_load:;

    for(n = 0; n< 200; n++) {
        int m;
        if(!noload) {
            for(m = 0; m < ((payload_bin_size + 7) & ~7); m+=8)
                lv2poke(PAYLOAD_BASE + (u64) m, addr[m>>3]);

            lv2poke(syscall_base + (u64) (379 * 8), PAYLOAD_BASE + 0x10ULL);
        }

        if(cfw == 0x341C) { // CFW 3.41 2EB128

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000AEA0ULL, 0x38600000); // sys 383 *

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000008644ULL, 0x38600001); // sys 409 *

            // enables sys_sm_set_fan_policy
            lv2poke32(0x8000000000008B40ULL, 0x38600001); // sys 389 *

            // enables sys_set_leds
            lv2poke32(0x8000000000008C08ULL, 0x38600001); // sys 386 *
            
        } else if(cfw == 0x355C) { // CFW 3.55 346570

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000B518ULL, 0x38600000); // sys 383 *

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000008CBCULL, 0x38600001); // sys 409 *

            // enables sys_sm_set_fan_policy
            lv2poke32(0x80000000000091B8ULL, 0x38600001); // sys 389 *

            // enables sys_set_leds
            lv2poke32(0x8000000000009280ULL, 0x38600001); // sys 386 *
            
        } else if(cfw == 0x421C) { // CFW 4.21

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383 *

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409 *

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389 *

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386 *
     
        } else if(cfw == 0x430C) { // CFW 4.30

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383 *

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409 *

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389 *

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386 *

        } else if(cfw == 0x431C) { // CFW 4.31

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386

        } else if(cfw == 0x440C) { // CFW 4.40

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
            
        } else if(cfw == 0x441C) { // firmware 4.41

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
    } else if(cfw == 0x446C) { // firmware 4.46

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386

    } else if(cfw == 0x450C) { // firmware 4.50

            // enables sys_game_get_temperature
            lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383

            // enables sys_sm_get_fan_policy
            lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

            // enables sys_sm_set_fan_policy
            lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

            // enables sys_set_leds
            lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
    
    } else if(cfw == 0x453C) { // firmware 4.53

        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386

    }

        sysUsleep(10000);
    }

   
}

volatile int temp_enable = 1;
volatile int set_main_flag = 0;

int cleds = 0, leds = 0;
int cspeed = 0x33;

u32 speed_table[8] = {
    0x4d,     // < temp_control0
    0x54,     // temp_control0 => temp_control1
    0x60,     // temp_control0 <= temp_control1
    0x68,     // >= temp_control1
    0x70,     // >= temp_control2
    0x78,     // >= temp_control3
    0xA0,     // >= temp_control4
    0xff      // >= temp_control5 

};

u32 temp_control[6] = {
    62,
    68,
    70,
    72,
    75,
    80
};


int speed_up = 0;
u64 payload_ctrl;
int stopped = 0;

static void fan_manager()
{
    int n;
    static int alive_cont = 5;
    

    static int cpu_temp = 0;
    static int rsx_temp = 0;
    

    static int step = 0;
    static int step2 = 0;

    step2++;
    if(step2 >=5) { // in Mode 1 copy the datas from the payload area (same method as PFAN)
        step2 = 0;
        
        if((temp_enable & 127)!=2 && lv2peek32(PAYLOAD_BASE) == 0x50534D45) { // copy datas from payload

            payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;
            cspeed = lv2peek32(payload_ctrl) & 0xff;
            payload_ctrl+= 0x4ULL;
            temp_enable = lv2peek32(payload_ctrl)!=0;
            if(!temp_enable) {set_main_flag = 1;}
            payload_ctrl+= 0x8ULL;
            for(n = 0; n < 7; n++) {
                speed_table[n] = lv2peek32(payload_ctrl);
                payload_ctrl+= 0x4ULL;
            }
            for(n = 0; n < 6; n++) {
                temp_control[n] = lv2peek32(payload_ctrl);
                payload_ctrl+= 0x4ULL;
                  
            }

        }
    
    }

    if(temp_enable && step2 == 1) {
        temp_enable^=128;
        stopped = 0;

        if(step == 0) {
            
            n= sys_game_get_temperature(0, (void *) &cpu_temp);
            if(n < 0) cpu_temp = 0;
        } else {
            n= sys_game_get_temperature(1, (void *) &rsx_temp);
            if(n < 0) rsx_temp =0;
        }

        step^=1;

        int speed = speed_table[0];
        int temp = cpu_temp;
   
        if(rsx_temp > temp) temp = rsx_temp;

        if(temp >= temp_control[1]) speed_up = 1;

        if(temp >= temp_control[5]) speed = speed_table[7];
        else if(temp >= temp_control[4]) speed = speed_table[6];
        else if(temp >= temp_control[3]) speed = speed_table[5];
        else if(temp >= temp_control[2]) speed = speed_table[4];
        else if(temp >= temp_control[1]) speed = speed_table[3];
        else if(temp >= temp_control[0] && temp < temp_control[1]) {
            if(speed_up) speed = speed_table[2]; else speed = speed_table[1];
        } else speed_up = 0;

        if(speed !=cspeed && temp_enable) {
            if(sys_sm_set_fan_policy(0, 2, speed)==0) {
                cspeed = speed;
                if((temp_enable & 127)!=2 && lv2peek32(PAYLOAD_BASE) == 0x50534D45) {
                    payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;
                    lv2poke32(payload_ctrl, cspeed); 
                }
            }
        }

        if(temp >= temp_control[4]) leds = 3;
        else if(temp >= temp_control[2]) leds = 2;
        else leds = 1;

        alive_cont--; if (alive_cont == 0) {leds = 0; alive_cont= 5; cspeed = 0;}


        if(temp_enable && leds != cleds && cleds != 0x10) {
          
           switch(leds) {
              case 0:          
                sys_set_leds(2, 0);
                sys_set_leds(1, 1);
                break;

              case 1:          
                sys_set_leds(2, 1);
                sys_set_leds(1, 1);
                break;

              case 2:          
                sys_set_leds(2, 2);
                sys_set_leds(1, 1);
                break;

              case 3:          
                sys_set_leds(2, 2);
                sys_set_leds(1, 2);
                break;
           }
        
            cleds =leds;
        }
    } else if(!temp_enable) {
        
        if(!set_main_flag) {
            sys_sm_set_fan_policy(0, 1, 0x5f);
            set_main_flag = 1;
            stopped = 1;
        } else stopped = 1;
    }

}

static void thread_temp(void *arg)
{

    while(1) {

        fan_manager();
        sysUsleep(400000);
        sysThreadYield();
        
    }  
}


volatile int usb_enable = 0;

char usb_pdevice[]= "/dev_usb000";

char usb_path[]= "/dev_usb000/nosleep";


u32 usb_data[4] = {0, 1, 2, 3};
u32 usb_timer = 60;

u32 usb_device = 0;
int doit = 0;
int no_multitheaded = 0;

int set_usb_signal = 0;

void UsbWakeup_tick()
{
    sysFSStat stat;
    s32 fd;
    u64 writed;
    int ret;
    int n;

    doit = 0;

        if(usb_enable) {
            usb_data[0]++;
            usb_data[1]^= usb_data[0] ^ 0x43651895;
            usb_data[2]^= usb_data[1] ^ 0x1a85f243;
            usb_data[3]=  usb_data[1] ^ usb_data[2];

            if(usb_enable == 2) n = 0; else n = usb_device; // usb_enable = 2 => scan all devices 

            while(1) {
                
                if(n >= 9) {
                    ret= sysLv2FsStat("/dev_bdvd", &stat);
                } else {
                    usb_pdevice[10]= 48 + n;
                    usb_path[10]= 48 + n;

                    ret= sysLv2FsStat(usb_pdevice, &stat);
                }

                if(ret == 0) {
                  
                    if(n>=9) {
                        ret= sysLv2FsStat("/dev_bdvd/nosleep", &stat);
                        if(ret == 0) 
                            ret = sysLv2FsOpen("/dev_bdvd/nosleep", SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);
                    } else {
                        ret= sysLv2FsStat(usb_path, &stat); // only write in devices with "nosleep" file
                        if(ret == 0) 
                            ret = sysLv2FsOpen(usb_path, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);
                    }

                    if(ret == 0) {

                        if(!doit) {
                            cleds = 0x10;
                            sys_set_leds(2, 0);
                            sys_set_leds(1, 0);
                        }

                        doit = 1;
                        sysLv2FsChmod(usb_path, FS_S_IFMT | 0777);
                        sysLv2FsWrite(fd, &usb_data, 16ULL, &writed);
                        sysLv2FsClose(fd);
                    }
                    
                }

                if(usb_enable == 1) break;
                    else {n++; if(n>=10) {n=0; break;}}

            }

            if(doit) {
                if(!no_multitheaded) {
                    sysSleep(2);
                    cleds = 0xff;
                    sys_set_leds(2, 1);
                    sys_set_leds(1, 1);
                } else set_usb_signal = 1;
            }

            
        }

}

static void thread_UsbWakeup(void *arg)
{
    int count_timer = 0;

    while(1) {

        sysThreadYield();
        sysSleep(10);
        count_timer+= 10;
        if(count_timer < usb_timer) continue;
        else count_timer= 0;

        UsbWakeup_tick();

    }
}

sys_ppu_thread_t temp_id = -1;
sys_ppu_thread_t usbwakeup_id = -1;

sys_ppu_thread_t main_id = -1;

u32 sleep1 = 0;
u32 sleep2 = 0;
u32 sleep3 = 0;

s32 main(s32 argc, const char* argv[]) 
{
    patchs_lv2();

    u64 command;
    u8 cmd;
    u8 mode;
    u8 arg0;
    u8 arg1;
    u64 addr;
    int n;
    u32 m_sleep = 1000;
    int count_timer = 0;
    
    if(sysThreadCreate(&temp_id, thread_temp, NULL, 3069, 0x1000, THREAD_JOINABLE, "FanCtrl_Thread") < 0) no_multitheaded = 1;
    else 
       sysThreadCreate(&usbwakeup_id, thread_UsbWakeup, NULL, 3070, 0x1000, THREAD_JOINABLE, "UsbWakeup_Thread");
    
    lv2poke(LV2_SM_CMD_ADDR, 0x5501000000000100ULL);

    sysThreadGetId(&main_id);
    sysThreadSetPriority(main_id, 3071);

    while(1) {
        
        if(no_multitheaded) {
            sysUsleep(1000);
            sleep1+= 1000;
            sleep2+= 1000;
            sleep3+= 1000;
           
            if(sleep2 >= 400000 && (!set_usb_signal || !usb_enable)) {
                sleep2 = 0;
                fan_manager();
            }

            if(usb_enable && set_usb_signal) {
                
                if(sleep3 >= 2000000) {
                    cleds = 0xff;
                    sys_set_leds(2, 1);
                    sys_set_leds(1, 1);
                    sleep3 = 0;
                    set_usb_signal = 0;
                }

            } else if(sleep3 >= 1000000) {
                sleep3 = 0;
                count_timer+= 1;
                if(count_timer >= usb_timer) {
                    count_timer= 0;
                    UsbWakeup_tick();
                }
            }

            if(sleep1 >= m_sleep) {
                sleep1 = 0;
               
            } else 
                continue;

        } else sysUsleep(m_sleep);

        command =lv2peek(LV2_SM_CMD_ADDR);
        cmd = (u8)  ((command>>56ULL) & 0xFF);
        mode = (u8) ((command>>48ULL) & 0xFF);
        arg0 = (u8) ((command>>40ULL) & 0xFF);
        arg1 = (u8) ((command>>32ULL) & 0xFF);
        addr = (command & 0xFFFFFFFFULL) | 0x8000000000000000ULL;

        switch(cmd) {

            case 0xAA: // adjust temperature mode/table
                if(mode == 0) { // disable
                    temp_enable = 0;
                    sys_set_leds(2, 0);
                    sys_set_leds(1, 1);
                    set_main_flag = 0;

                    while(1) {
                        sysUsleep(10000);
                        if(no_multitheaded) fan_manager();
                        if(set_main_flag) break;
                    }

                    lv2poke(LV2_SM_CMD_ADDR, 0x55AA000000000000ULL);

                } else { // enable
                    temp_enable = 0;

                    sys_set_leds(2, 1);
                    sys_set_leds(1, 1);

                    if(mode == 2) { // mode 2 copy table speeds/temperatures from LV2

                        for(n = 0; n < 8; n++) {
                            speed_table[n] = lv2peek32(addr);
                            addr+= 0x4ULL;
                        }
                        for(n = 0; n < 6; n++) {
                            temp_control[n] = lv2peek32(addr);
                            addr+= 0x4ULL;
                              
                        }

                    }

                    cleds = 0xff;
                    cspeed = 0x33;

                    temp_enable = mode;

                    lv2poke(LV2_SM_CMD_ADDR, 0x55AA000000000000ULL);
                }

            break;

            case 0xBB: // USB Wakeup method

                if(mode == 0) usb_enable = 0;
                else {
                    usb_enable = 0;
                    usb_timer = 10 + ((u32) (arg0)) * 10;

                    if(mode == 1) {
                    
                        if(arg1 > 9) arg1 = 9;
                        
                        usb_device = arg1;
                        
                        usb_enable = 1;
                    } else usb_enable = 2;
                }

                lv2poke(LV2_SM_CMD_ADDR, 0x55BB000000000000ULL);

            break;
            case 0xCC: // thread priority / main sleep
                
                if(mode == 0) sysThreadSetPriority(temp_id, (s32)  addr);
                else if(mode == 1) sysThreadSetPriority(usbwakeup_id, (s32)  addr);
                else if(mode == 0xff) sysThreadSetPriority(main_id, (s32)  addr);
                else if(mode == 0xfe) m_sleep = addr;
                lv2poke(LV2_SM_CMD_ADDR, 0x55CC000000000000ULL);

            break;
            case 0xFF: // status               // -> version
                lv2poke(LV2_SM_CMD_ADDR, 0x55FF010000000000ULL |
                    (u64) ((stopped!=0) | ((temp_enable & 0xf)<<4) | ((usb_enable & 0xf)<<8)));
            break;
            default:

                if(cmd != 0x55) lv2poke(LV2_SM_CMD_ADDR, 0x5500000080010002ULL);

            break;

        }

    }

	return 0;
}

