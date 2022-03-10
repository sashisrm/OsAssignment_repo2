#include <types.h>
#include <context.h>
#include <lib.h>
#include <memory.h>
#include <entry.h>

#define ENABLE_DEBUG 1


// Task 1
/*
** Function:
**      gCsn - Get current segment name
** Parameters:
**      ssa - Segment start address
**      sea - Segment end address
**      csn - Current segement name
** Returns:
**      sni - Segment name index
*/
int gCsn(unsigned long ssa, unsigned long sea, unsigned char *csn) {
    u8 _sni = 0x00;

    if (ssa >= CODE_START && sea < RODATA_START) {
        strcat(csn, "CODE");
        _sni= 4;
    } else if (ssa >= DATA_START && sea < MMAP_START) {
        strcat(csn, "DATA");
        _sni = 4;
    } else if (ssa >= MMAP_START && sea < STACK_START) {
        strcat(csn, "MMAP");
        _sni = 4;
    }  else if (ssa >= RODATA_START && sea < DATA_START) {
        strcat(csn, "RO DATA");
        _sni = 7;
    } else {
        strcat(csn, "STACK");
        _sni = 5;
    }

    return _sni;
}

/*
** Function:
**      print_pmaps - Prints pmaps
** Parameters:
**      ctx - Context
** Returns:
**      sni - Segment name index
*/
int print_pmaps(struct exec_context *ctx)
{
    u8 _i = 0x00;
    for (_i = 0; _i < MAX_MM_SEGS; _i++) {

        /*
        ** _csi - Current segment permission index
        ** _csp - Current segement permission
        */
        u8 _csi = 0x00;
        unsigned char _csp[10] = {0x00};

        if (ctx->mms[_i].next_free > ctx->mms[_i].start) {

            /*
            ** R - Read
            */
            _csp[_csi++] = ctx->mms[_i].access_flags & MM_RD ? 'R' : '-';

            /*
            ** W - Write
            */
            _csp[_csi++] = ctx->mms[_i].access_flags & MM_WR ? 'W' : '-';

            /*
            ** X - eXecute
            */
            _csp[_csi++] = ctx->mms[_i].access_flags & MM_EX ? 'X' : '-';

            /*
            ** _csn - Current segement name
            ** _csl - Current segment name length
            */
            unsigned char _csn[10] = {0x00};
            u8 _csl = gCsn(ctx->mms[_i].start, ctx->mms[_i].end, _csn);

            /*
            ** If 'STACK'
            **  then get full (bottom to top) start and end addresses.
            ** Else
            **  then
            **      If 'True'
            **       then process has Dynamic Memory (DMM) stored in linked list.
            **       _nvm - Next VM
            **       _vsp - VM segment permissions
            **       _vsi - VM segment permissions index
            */
            if (strcmp(_csn, "STACK") != 0X00) {
                printk("%x %x %s %s \n", ctx->mms[_i].start, ctx->mms[_i].next_free, _csp, _csn);
            } else {
                if (ctx->vm_area != NULL) {
                    struct vm_area *_nvm = ctx->vm_area;

                    while (_nvm != NULL) {
                        unsigned char _vsp[10] = {0x00};
                        u8 _vsi = 0x00;

                        /*
                        ** R - Read
                        */
                        _vsp[_vsi++] = _nvm->access_flags & MM_RD ? 'R' : '-';

                        /*
                        ** Write - Write
                        */
                        _vsp[_vsi++] = _nvm->access_flags & MM_WR ? 'W' : '-';

                        /*
                        ** X - eXecute
                        */
                        _vsp[_vsi++] = _nvm->access_flags & MM_EX ? 'X' : '-';

                        printk("%x %x %s %s \n", _nvm->vm_start, _nvm->vm_end, _vsp, "MMAP");

                        _nvm = _nvm->vm_next;
                    }
                }

                /*
                ** Display stack segement. (Start and end addresses)
                */
                printk("%x %x %s %s \n", ctx->mms[_i].next_free, ctx->mms[_i].end, _csp, _csn);
            }
        }
    }

    return 0;
}

// Task 2
/*
** Function:
**      print_page_table - Prints page table
** Parameters:
**      ctx - Context
**  adr - Address
** Returns:
**      0
*/
int print_page_table(struct exec_context *ctx, u64 adr) {

    /*
    ** _vab - Virtual address base
    ** _ost - Offset address
    ** _ent - Entry address
    ** _nba - Next base address
    */
    u64 *_vab = (u64 *)osmap(ctx->pgd);

    /*
    ** Displays: Level 1
    */
    u64 _ost = (adr & PGD_MASK) >> PGD_SHIFT;
    u64 *_ent = _vab + _ost;
    u64 *_nba = (u64 *)(*_ent & 0xFFFF000);

    if (*_ent != 0x00) {
        printk("L1-entry:%x, *(L1-entry):%x, L2-VA-Base:%x\n", _ent, *_ent, _nba);
    } else {
        printk("L1-entry: Not found.\n");
    }

    /*
    ** Displays: Level 2
    */
    _ost = (adr & PUD_MASK) >> PUD_SHIFT;
    _ent = _nba + _ost;
    _nba = (u64 *)(*_ent & 0xFFFF000);

    if (*_ent != 0x00) {
        printk("L2-entry:%x, *(L2-entry):%x, L3-VA-Base:%x\n", _ent, *_ent, _nba);
    } else {
        printk("L2-entry: Not found.\n");
    }

    /*
    ** Displays: Level 3
    */
    _ost = (adr & PMD_MASK) >> PMD_SHIFT;
    _ent = _nba + _ost;
    _nba = (u64 *)(*_ent & 0xFFFF000);

    if (*_ent != 0x00) {
        printk("L3-entry:%x, *(L3-entry):%x, L4-VA-Base:%x\n", _ent, *_ent, _nba);
    } else {
        printk("L3-entry: Not found.\n");
    }

    /*
    ** Displays: Level 4
    */
    _ost = (adr & PTE_MASK) >> PTE_SHIFT;
    _ent = _nba + _ost;
    u64 PFN = (*_ent & 0xFFFF000) >> PTE_SHIFT;

    if (*_ent != 0x00) {
        printk("L4-entry:%x, *(L4-entry):%x, PFN:%x\n", _ent, *_ent, PFN);
    } else {
        printk("L4-entry: Not found.\n");
    }

    return 0;
}

// Task 3
/*
** Function:
**      do_dfork - Prints page table
** Parameters:
**      cha - Child stack address
** Returns:
**      pid - Process id.
*/
int do_dfork(u64 cha) {

    /*
    ** _nct - New context
    ** _cct - Current context
    */

    u32 pid;

    struct exec_context *_nct = get_new_ctx();
    struct exec_context *_cct = get_current_ctx();

    //Copy PPID of the parent process
    /*
    ** Replicate parent process id,
    ** then replicate the type,
    ** then set new process state,
    ** then replicate used memory,
    ** then display child stack address.
    */
    _nct->ppid = _cct->ppid;
    _nct->type = _cct->type;
    _nct->state = 1;
    _nct->used_mem = _cct->used_mem;

    printk("Child Stack Address: %x\n", &cha);

    /*
    ** Bunch of outputs.
    */
    if(ENABLE_DEBUG) {

        /*
        ** do_dfork( u64 child_stack) implementation in dfork.c;
        ** where child_stack corresponds to address of child process stack (1 MB in size),
        ** which is passed from the user program in user/init.c.
        */
        printk("Parent - Start  ################################\n");
        printk("    PID: %x\n", _cct->pid);
        printk("    PPID: %x\n", _cct->ppid);
        printk("    Type: %x\n", _cct->type);
        printk("    State: %x\n", _cct->state);
        printk("    Used Memory: %x\n", _cct->used_mem);
        printk("    PGD: %x\n", _cct->pgd);
        printk("    Stack PFN: %x\n", _cct->os_stack_pfn);
        printk("    OS RSP: %x\n", _cct->os_rsp);

        /*
        ** Memory Management System (MMS)
        */

        /*
        ** Displays parent virtual memory area.
        */
        printk("    VM Area Location: %x\n", &_cct->vm_area);
        printk("    Name: %s\n", _cct->name);
        /*
        ** Registered user.
        */
        printk("    Pending Signals: %x\n", _cct->pending_signal_bitmap);
        printk("    Signal Handler's Location: %x\n", &_cct->sighandlers);
        printk("    Ticks to Sleep: %x\n", _cct->ticks_to_sleep);
        printk("    Alarm Config Time: %x\n", _cct->alarm_config_time);
        printk("    Ticks to Alarm: %x\n", _cct->ticks_to_alarm);
        printk("    File Location: %x\n", &_cct->files);

        printk("Parent - End    ################################\n");

        /*
        ** Child process outputs.
        */
        printk("Child - Start   ################################\n");
        printk("    PID: %x\n", _nct->pid);
        printk("    PPID: %x\n", _nct->ppid);
        printk("    Type: %x\n", _nct->type);
        printk("    State: %x\n", _nct->state);
        printk("    Used Memory: %x\n", _nct->used_mem);
        printk("    PGD: %x\n", _nct->pgd);
        printk("    Stack PFN: %x\n", _nct->os_stack_pfn);
        printk("    OS RSP: %x\n", _nct->os_rsp);

        /*
        ** Memory Management System (MMS)
        */

        /*
        ** Displays child virtual memory area.
        */
        printk("    VM Area Location: %x\n", &_nct->vm_area);
        printk("    Name: %s\n", _nct->name);
        /*
        ** Registered user.
        */
        printk("    Register R15: %x\n", _nct->regs.r15);

        printk("    Pending Signals: %x\n", _nct->pending_signal_bitmap);
        printk("    Signal Handler's Location: %x\n", &_nct->sighandlers);
        printk("    Ticks to Sleep: %x\n", _nct->ticks_to_sleep);
        printk("    Alarm Config Time: %x\n", _nct->alarm_config_time);
        printk("    Ticks to Alarm: %x\n", _nct->ticks_to_alarm);
        printk("    File Location: %x\n", &_nct->files);

        printk("Child - End     ################################\n");
    }

    setup_child_context(_nct);
    return pid;
}