#include <stdint.h>
#include <mm/physman.h>
#include <mm/paging.h>
#include <mm/mm.h>
#include <utils/print.h>
#include <utils/string.h>
#include <utils/spinlock.h>

header_t *head, *tail;

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

header_t *get_free_block(size_t size)
{
    header_t *curr = head;
    while(curr) {
        if(curr->s.is_free && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

size_t last_top = 0;

void * ksbrk(intptr_t increment) {
    if(increment == 0) {
        return (void *)last_top;
    } else if(increment < 0) {
        return NULL;
    }

    size_t pages = ROUND_UP(increment, PAGE_SIZE) / PAGE_SIZE;
    void *out = (void *)physman_alloc(pages);

    out += VIRT_PHYS_BASE;
    last_top = (size_t)out;

    return (void *)out;
}

static spinlock_t global_malloc_lock;

void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;
    if(!size)
        return NULL;
    spinlock_lock(&global_malloc_lock);
    header = get_free_block(size);
    if(header) {
        header->s.is_free = 0;
        spinlock_release(&global_malloc_lock);
        return (void*)(header + 1);
    }
    total_size = sizeof(header_t) + size;
    block = ksbrk(total_size);
    if(block == (void*) -1) {
        spinlock_release(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if(!head)
        head = header;
    if(tail)
        tail->s.next = header;
    tail = header;
    spinlock_release(&global_malloc_lock);
    return (void*)(header + 1);
}

void free(void *block)
{
    header_t *header, *tmp;
    void *programbreak;

    if(!block)
        return;
    spinlock_lock(&global_malloc_lock);
    header = (header_t*)block - 1;

    programbreak = ksbrk(0);
    if((char*)block + header->s.size == programbreak) {
        if(head == tail) {
            head = tail = NULL;
        } else {
            tmp = head;
            while(tmp) {
                if(tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        ksbrk(0 - sizeof(header_t) - header->s.size);
        spinlock_release(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    spinlock_release(&global_malloc_lock);
}

void *calloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    if(!num || !nsize)
        return NULL;
    size = num * nsize;
    if(nsize != size / num)
        return NULL; //Wtf happened here, seems overflow
    block = malloc(size);
    if(!block)
        return NULL;
    memset(block, 0, size);
    return block;          
}

void *realloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    if (!block || !size)
        return malloc(size);
    header = (header_t*)block - 1;
    if(header->s.size >= size)
        return block;
    ret = malloc(size);
    if(ret) {
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}