#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define BUFFER_SIZE 8

struct RingBuffer {
    uint8_t buffer[BUFFER_SIZE]; 
    uint8_t head;                
    uint8_t tail;                
    uint8_t count;               
};

struct RingBuffer rb;

void ringbuf_init() {
    rb.head  = 0;
    rb.tail  = 0;
    rb.count = 0;
}

bool ringbuf_is_full() {
    return rb.count == BUFFER_SIZE;
}

bool ringbuf_is_empty() {
    return rb.count == 0;
}


bool ringbuf_write(uint8_t data) {
    if (ringbuf_is_full()) {
        return false;
    }
    rb.buffer[rb.head] = data;

    /*
     head using bitwise AND instead of modulo %
     
     Why it is faster ?
     
     On MCUs without a hardware divider, the % operator compiles to a slow software division routine that can take
     20-100+ cycles. A single bitwise AND executes in 1 cycle on every architecture.
     
     Why does it only work when BUFFER_SIZE is a power of 2?
     
     When N = 2^k, (N - 1) is a bitmask of k ones (e.g. 8-1 = 0b00000111).
     ANDing any index with this mask is mathematically identical to index % N,
     so the wrap-around is correct and free of any division.
     
     */
    rb.head = (rb.head + 1) & (BUFFER_SIZE - 1);
    rb.count++;
    return true;
}

bool ringbuf_read(uint8_t *data) {
    if (ringbuf_is_empty()) {
        return false;
    }
    *data = rb.buffer[rb.tail];

    /* Same bitwise AND wrap-around as in ringbuf_write as per above */
    rb.tail = (rb.tail + 1) & (BUFFER_SIZE - 1);
    rb.count--;
    return true;
}

uint8_t ringbuf_count() {
    return rb.count;
}

int main() {
    ringbuf_init();

    // Write 8 bytes (0x41 to 0x48) 
    
    uint8_t write_data[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
    for (int i = 0; i < 8; i++) {
        if (ringbuf_write(write_data[i])) {
            printf("[WRITE] 0x%02X -> OK (count=%d)%s\n",
                   write_data[i], ringbuf_count(),
                   ringbuf_is_full() ? " FULL" : "");
        } else {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", write_data[i]);
        }
    }


    //Attempt to write one more byte -- must fail
    if (ringbuf_write(0x99)) {
        printf("[WRITE] 0x99 -> OK (count=%d)\n", ringbuf_count());
    } else {
        printf("[WRITE] 0x99 -> FAIL (buffer full)\n");
    }


    // Read 3 bytes (expected: 0x41, 0x42, 0x43) 
    uint8_t read_data;
    for (int i = 0; i < 3; i++) {
        if (ringbuf_read(&read_data)) {
            printf("[READ] -> 0x%02X (count=%d)\n", read_data, ringbuf_count());
        } else {
            printf("[READ] -> FAIL (buffer empty)\n");
        }
    }

    // Write 3 new bytes -- reuses the 3 freed slots 
    
    uint8_t new_write_data[] = {0x49, 0x4A, 0x4B};
    for (int i = 0; i < 3; i++) {
        if (ringbuf_write(new_write_data[i])) {
            printf("[WRITE] 0x%02X -> OK (count=%d)%s\n",
                   new_write_data[i], ringbuf_count(),
                   ringbuf_is_full() ? " FULL" : "");
        } else {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", new_write_data[i]);
        }
    }

    // Read all remaining 8 bytes 
    
    while (!ringbuf_is_empty()) {
        if (ringbuf_read(&read_data)) {
            printf("[READ] -> 0x%02X (count=%d)\n", read_data, ringbuf_count());
        }
    }

    // Attempt to read from empty buffer -- must fail 
    
    if (ringbuf_read(&read_data)) {
        printf("[READ] -> 0x%02X (count=%d)\n", read_data, ringbuf_count());
    } else {
        printf("[READ] (empty) -> FAIL (buffer empty)\n");
    }
    return 0;
}
