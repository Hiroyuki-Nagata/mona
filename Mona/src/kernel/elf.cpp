/*!
    \file  elf.cpp
    \brief elf utility

    Copyright (c) 2003 Higepon
    All rights reserved.
    License=MIT/X Licnese

    \author  HigePon
    \version $Revision$
    \date   create:2003/09/06 update:$Date$
*/

#include "elf.h"
#include "string.h"
#include "global.h"
#include "io.h"
#include "syscalls.h"

#define SHARED_MM_ERROR -1
#define FAT_INIT_ERROR  -2
#define FAT_OPEN_ERROR  -3

#define MAX_IMAGE_SIZE (4096 * 150)

#define ORG 0xA0000000

int loadProcess(byte* image, dword size, dword entrypoint, const char* path, const char* name, bool isUser, CommandOption* list)
{
    /* shared ID */
    static dword sharedId = 0x2000;
    sharedId++;

    bool   isOpen;
    bool   isAttaced;

    /* attach Shared to this process */
    while (Semaphore::down(&g_semaphore_shared));
    isOpen    = SharedMemoryObject::open(sharedId, MAX_IMAGE_SIZE);
    isAttaced = SharedMemoryObject::attach(sharedId, g_currentThread->process, 0x80000000);
    Semaphore::up(&g_semaphore_shared);
    if (!isOpen || !isAttaced) return 4;

    /* create process */
    enter_kernel_lock_mode();
    Process* process = ProcessOperation::create(isUser ? ProcessOperation::USER_PROCESS : ProcessOperation::KERNEL_PROCESS, path, name);

    /* attach binary image to process */
    while (Semaphore::down(&g_semaphore_shared));
    isOpen    = SharedMemoryObject::open(sharedId, MAX_IMAGE_SIZE);
    isAttaced = SharedMemoryObject::attach(sharedId, process, ORG);
    Semaphore::up(&g_semaphore_shared);
    if (!isOpen || !isAttaced) return 5;

    memcpy((byte*)0x80000000, image, size);

    /* detach from this process */
    while (Semaphore::down(&g_semaphore_shared));
    SharedMemoryObject::detach(sharedId, g_currentThread->process);
    Semaphore::up(&g_semaphore_shared);

    /* set arguments */
    if (list != NULL)
    {
        char* p;
        CommandOption* option;
        List<char*>* target = process->getArguments();

        for (option = list->next; option; option = option->next)
        {
            p = new char[32];
            strncpy(p, option->str, 32);
            target->add(p);
        }
    }

    /* now process is loaded */
    Thread*  thread = ThreadOperation::create(process, entrypoint);
    g_scheduler->join(thread);
    exit_kernel_lock_mode();
    return 0;
}

int loadProcess(const char* path, const char* name, bool isUser, CommandOption* list)
{
    char filepath[128];

    strncpy(filepath, path, sizeof(filepath));

    /* shared ID */
    static dword sharedId = 0x1000;
    sharedId++;

    int    fileSize;
    int    readTimes;
    bool   isOpen;
    bool   isAttaced;
    dword entrypoint;
    ELFLoader *loader;
    byte* buf;

    /* only one process can use fd */
    while (Semaphore::down(&g_semaphore_fd));
    g_fdcdriver->motor(ON);
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();
    g_fdcdriver->recalibrate();

    /* file open */
    if (!(g_fs->open(filepath, 1)))
    {
        g_fdcdriver->motorAutoOff();
        Semaphore::up(&g_semaphore_fd);
        return 1;
    }

    /* get file size and allocate buffer */
    fileSize  = g_fs->size();

    readTimes = (fileSize + 512 -1) / 512;
    buf       = (byte*)malloc(512 * readTimes);

    if (buf == NULL)
    {
        g_fdcdriver->motorAutoOff();
        Semaphore::up(&g_semaphore_fd);
        return 2;
    }
    memset(buf, 0, 512 * readTimes);

    /* read */
    if (!g_fs->read(buf, fileSize))
    {
            free(buf);
            g_fdcdriver->motorAutoOff();
            Semaphore::up(&g_semaphore_fd);
            return g_fs->getErrorNo();
    }

    /* close */
    if (!g_fs->close())
    {
        g_fdcdriver->motorAutoOff();
        Semaphore::up(&g_semaphore_fd);
    }

    g_fdcdriver->motorAutoOff();
    Semaphore::up(&g_semaphore_fd);

    /* attach Shared to this process */
    while (Semaphore::down(&g_semaphore_shared));
    isOpen    = SharedMemoryObject::open(sharedId, MAX_IMAGE_SIZE);
    isAttaced = SharedMemoryObject::attach(sharedId, g_currentThread->process, 0x80000000);
    Semaphore::up(&g_semaphore_shared);
    if (!isOpen || !isAttaced)
    {
        free(buf);
        return 4;
    }

    /* load */
    loader = new ELFLoader();
    loader->prepare((dword)buf);
    entrypoint = loader->load((byte*)0x80000000);
    delete(loader);
    free(buf);

    /* create process */
    enter_kernel_lock_mode();
    Process* process = ProcessOperation::create(isUser ? ProcessOperation::USER_PROCESS : ProcessOperation::KERNEL_PROCESS, path, name);

    /* attach binary image to process */
    while (Semaphore::down(&g_semaphore_shared));
    isOpen    = SharedMemoryObject::open(sharedId, MAX_IMAGE_SIZE);
    isAttaced = SharedMemoryObject::attach(sharedId, process, ORG);
    Semaphore::up(&g_semaphore_shared);
    if (!isOpen || !isAttaced)
    {
        return 5;
    }

    /* detach from this process */
    while (Semaphore::down(&g_semaphore_shared));
    SharedMemoryObject::detach(sharedId, g_currentThread->process);
    Semaphore::up(&g_semaphore_shared);

    /* set arguments */
    if (list != NULL)
    {
        char* p;
        CommandOption* option;
        List<char*>* target = process->getArguments();

        for (option = list->next; option; option = option->next)
        {
            p = new char[32];
            strncpy(p, option->str, 32);
            target->add(p);
        }
    }

    /* now process is loaded */
    Thread*  thread = ThreadOperation::create(process, entrypoint);
    g_scheduler->join(thread);
    exit_kernel_lock_mode();
    return 0;
}

ELFLoader::ELFLoader() : errorCode_(0), header_(NULL), pheader_(NULL)
{
}

ELFLoader::~ELFLoader()
{
}

int ELFLoader::prepare(dword elf)
{
    int size = 0;

    /* check ELF header */
    header_  = (ELFHeader*)elf;
    if (!isValidELF()) return errorCode_;

    /* Program Header */
    pheader_ = (ELFProgramHeader*)((dword)header_ + header_->phdrpos);

    /* Section Header */
    sheader_ = (ELFSectionHeader*)((dword)header_ + header_->shdrpos);

    /* get size of image */
    dword start = ORG;
    for (int i = 0; i < header_->shdrcnt; i++)
    {
        if (sheader_[i].address >= start)
        {
            start = sheader_[i].address;
            size = start - ORG + 1 + sheader_[i].size;
        }
    }

    if (size > MAX_IMAGE_SIZE)
    {
        g_console->printf("alert!!! : ELF size over size = %d \n", size);
    }

    return size;
}

#define SH_NOBITS 8
dword ELFLoader::load(byte* toAddress)
{
    for (int i = 0; i < header_->phdrcnt; i++)
    {
#if 1   // yui pheader_[0]->virtualaddr != ORGの場合にロードに失敗する
        if (pheader_[i].type == PT_LOAD) {
            memcpy((void*)(pheader_[i].virtualaddr - ORG + toAddress), (void*)((dword)header_ + pheader_[i].offset), pheader_[i].filesize);
            if (pheader_[i].memorysize > pheader_[i].filesize) {
                memset((void*)(pheader_[i].virtualaddr + pheader_[i].filesize - ORG + toAddress), 0, pheader_[i].memorysize - pheader_[i].filesize);
            }
        }
#else
        if (pheader_[i].type == PT_LOAD && pheader_[i].filesize == pheader_[i].memorysize)
        {
            memcpy((void*)(toAddress + pheader_[i].virtualaddr - pheader_->virtualaddr), (void*)((dword)header_ + pheader_[i].offset), pheader_[i].filesize);
        }
        else if (pheader_[i].type == PT_LOAD && pheader_[i].filesize != pheader_[i].memorysize)
        {
            memcpy((void*)(toAddress + pheader_[i].virtualaddr - pheader_->virtualaddr), (void*)((dword)header_ + pheader_[i].offset), pheader_[i].memorysize);

            /* zero clear*/
            //memset((void*)(toAddress + pheader_[i].virtualaddr - header_->entrypoint + pheader_[i].filesize), 0, pheader_[i].memorysize - pheader_[i].filesize);
        }
#endif
    }

    for (int i = 0; i < header_->shdrcnt; i++)
    {
        /* .bss */
        if (sheader_[i].type == SH_NOBITS)
        {
            memset((void*)(toAddress + sheader_[i].address - ORG), 0, sheader_[i].size);
        }
    }

    return header_->entrypoint;
}

int ELFLoader::getErrorCode() const
{
    return errorCode_;
}

bool ELFLoader::isValidELF()
{
    /* check magic number */
    if (header_->magic[0] != 0x7F || header_->magic[1] != 'E'
        || header_->magic[2] != 'L' ||header_->magic[3] != 'F')
    {
        errorCode_ = ELF_ERROR_NOT_ELF;
        return false;
    }

    /* little endian, version 1, x86 */
    if (header_->clazz != 1 || header_->headerversion != 1 || header_->archtype != 3)
    {
        errorCode_ = ELF_ERORR_NOT_SUPPORTED_ELF;
        return false;
    }

    /* check executable */
    if (header_->type != 2)
    {
        errorCode_ = ELF_ERORR_NOT_EXECUTABLE;
        return false;
    }

    /* valid ELF */
    return true;
}
