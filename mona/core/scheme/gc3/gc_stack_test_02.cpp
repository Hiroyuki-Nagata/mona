/*!
    \file   gc_stack_test_02.cpp
    \brief

    Copyright (c) 2002-2007 Higepon.
    All rights reserved.
    License=MIT/X License

    \author  Higepon
    \version $Revision$
    \date   create:2007/07/14 update:$Date$
*/
#include "GCRecord.h"

extern GCRecord root;

void test()
{
    char* p = new char[32];
    char* q = new char;
}

void test2()
{
    short* r = new short;
    char* t = new char[5];
}

int dummy(int i)
{
    if (i > 100) return 100;
    return dummy(i + 1);
}

// this variable is refered, so gcc can't erase variables.
void this_variable_is_refered(void* p)
{
    volatile void* h = p;
    FILE* f = fopen("/dev/null", "w+");
    fprintf(f, "%x", p);
    fclose(f);
}

int main(int argc, char *argv[])
{
    gc_init();

    int x = 0x12345678;
    int* q = new int;
    test();
    test2();
    int y = 0x99998888;
    char* buf = new char[64];
    dummy(0);
    gc();
    int size = gc_record_size(&root);

    if (size == 2 && root.next->size == 64 && root.next->next->size == 4)
    {
        printf("[OK] %s\n", argv[0]);
        return 0;
    }
    else
    {
        printf("%s [NG]\n", argv[0]);
        FOREACH_GC_RECORD(&root, e)
        {
            printf("    not sweeped size = %d\n", e->size);
        }
        this_variable_is_refered(q);
        this_variable_is_refered(buf);
        return 1;
    }

}
