#include <assert.h>
#include <stdbool.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

struct Queue {
    size_t cap;
    atomic_size_t size;
    size_t head;
    size_t tail;
    int data[0];
};

struct Queue *q_new(size_t cap) {
    struct Queue *q = calloc(1, sizeof(*q) + sizeof(int) * (cap + 1));
    assert(q);
    q->cap = cap + 1;
    return q;
}

struct Arg {
    struct Queue *q;
    size_t n_items;
    unsigned consumer_id;
    atomic_size_t *n_consumed;
};

void *producer(void *data) {
    struct Arg *arg = data;
    struct Queue *q = arg->q;

    for (unsigned i = 0; i < arg->n_items;) {
        size_t size = atomic_load_explicit(&q->size, memory_order_acquire);
        size_t tail = q->tail;

        if (size == q->cap - 1)
            continue;

        q->tail = (q->tail + 1) % q->cap;
        if (!atomic_compare_exchange_strong_explicit(
                    &q->size,
                    &size,
                    size + 1,
                    memory_order_release,
                    memory_order_relaxed)) {
            q->tail = tail;
            continue;
        }

        q->data[tail] = i;
        printf("producer %u\n", i);

        i++;
    }

    return NULL;
}

void *consumer(void *data) {
    struct Arg *arg = data;
    struct Queue *q = arg->q;

    while (arg->n_items != atomic_load_explicit(arg->n_consumed, memory_order_relaxed)) {
        size_t size = atomic_load_explicit(&q->size, memory_order_acquire);
        size_t head = q->head;

        if (size == 0)
            continue;

        q->head = (q->head + 1) % q->cap;
        if (!atomic_compare_exchange_strong_explicit(
                    &q->size,
                    &size,
                    size - 1,
                    memory_order_release,
                    memory_order_relaxed)) {
            q->head = head;
            continue;
        }

        atomic_fetch_add_explicit(arg->n_consumed, 1, memory_order_relaxed);
        printf("consumer%u %u\n", arg->consumer_id, q->data[head]);
    }


    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3)
        errx(1, "usage: exec QUEUE_CAP N_ITEMS");

    pthread_t p, c1, c2;
    size_t cap, n_items;
    atomic_size_t n_consumed = 0;
    struct Queue *q;

    n_items = strtol(argv[2], NULL, 10);
    cap = strtol(argv[1], NULL, 10);
    q = q_new(cap);

    pthread_create(&p, NULL, producer,
            &(struct Arg){.q=q, .n_items=n_items});
    pthread_create(&c1, NULL, consumer,
            &(struct Arg){.q=q, .n_items=n_items, .consumer_id=1, .n_consumed=&n_consumed});
    pthread_create(&c2, NULL, consumer,
            &(struct Arg){.q=q, .n_items=n_items, .consumer_id=2, .n_consumed=&n_consumed});

    pthread_join(p, NULL);
    pthread_join(c1, NULL);
    pthread_join(c2, NULL);

    return 0;
}