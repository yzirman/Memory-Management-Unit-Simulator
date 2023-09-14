/* Written by Jonathan Zirman
 * Please Enjoy! */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define MAXLINE 1024
#define MAXARGS 10

typedef struct table_entry {
    int exists;
    int valid;
    int physical_page_number;
    uint64_t memory_location;
} table_entry;

char prompt[] = ">";
char command_line[MAXLINE];
int vmpc;
int pmpc;
int bytes_per_page;
char **command_array;
uint64_t *physical_memory;
uint64_t *disk;
table_entry page_table[100];
int *queue;
int front_of_queue;
int back_of_queue;
int total_pages;
int *dirty;
int *virtual_locations;

void build_command_line();

void execute_commands();

void read_byte();

void write_byte();

int Exit();

int powerofTwo(int pagesize);

void create_page(int physical_page_number_destination, int virtual_page_number);

void write_to_disk(int virtual_page_number, int physical_page_number);

void write_from_disk(int virtual_page_number_source, int physical_page_number_destination);

void enqueue(int page_number);

int dequeue();

void Posix_memalign(uint64_t *memptr, size_t alignment, size_t size);

void *Malloc(size_t size);

void *Calloc(size_t nmemb, size_t size);

void Free(void *ptr);

void unix_error(char *msg);

void app_error(char *msg);

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("%s", "error: incorrect number of command line arguments\n");
        exit(1);
    }
    bytes_per_page = atoi(argv[1]);
    vmpc = atoi(argv[2]);
    pmpc = atoi(argv[3]);

    if (powerofTwo(bytes_per_page) == 1) {
        printf("%s", "error: pagesize must be power of 2\n");
        exit(1);
    }
    dirty = Calloc(pmpc, sizeof(int));
    queue = Malloc(vmpc * sizeof(int));
    command_array = Malloc(sizeof(char *) * MAXARGS);
    physical_memory = Malloc(pmpc * sizeof(uint64_t));
    disk = Malloc(100 * sizeof(uint64_t));
    virtual_locations = Malloc(pmpc * sizeof(int *));
    front_of_queue = 0;
    back_of_queue = 0;

    //create "disk"
    for (int i = 0; i < 100; i++) {
        Posix_memalign(&disk[i], bytes_per_page, bytes_per_page);
    }
    while (TRUE) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(command_line, MAXLINE, stdin) == NULL) {
            app_error("fgets error");
        }
        build_command_line();
        execute_commands();
        fflush(stdout);
    }
}

void build_command_line() {
    int counter = 0;
    char *ptr = strtok(command_line, " ");
    command_array[counter++] = ptr;
    while((ptr = strtok(NULL, " ")) != NULL) {
        command_array[counter++] = ptr;
    }
}

void execute_commands() {
    if (strcmp(command_array[0], "exit\n") == 0) { //quit
        Exit();
    } else if (strcmp(command_array[0], "readbyte") == 0) { //read
        read_byte();
    } else if (strcmp(command_array[0], "writebyte") == 0) { //write
        write_byte();
    } else { //error
        app_error("error: unrecognized command");
    }
}

int Exit() {
    //free kodak
    int created = pmpc;
    if (total_pages < pmpc) {
        created = total_pages;
    }
    for (int i = 0; i < created; i++) {
        Free((void *)physical_memory[i]);
    }
    Free(dirty);
    Free(queue);
    Free(command_array);
    Free((void *)physical_memory);
    Free((void * )disk);
    Free(virtual_locations);
    exit(0);
}

int powerofTwo(int pagesize) {
    if (pagesize == 0)
        return 1;
    while(pagesize != 1) {
        if(pagesize % 2 != 0)
            return 1;
        pagesize /= 2;
    }
    return 0;
}

void read_byte() {
    uint64_t memory_location = strtol(command_array[1], NULL, 16);
    if (memory_location >= bytes_per_page * vmpc) {
        printf("readbyte: segmentation fault\n");
        return;
    }
    uint64_t offset = memory_location % bytes_per_page;
    int virtual_page_number = memory_location / bytes_per_page;
    table_entry entry = page_table[virtual_page_number];
    if (entry.exists == 0 && total_pages >= pmpc) {
        int physical_page_number = dequeue();
        entry.physical_page_number = physical_page_number;
        entry.memory_location = physical_memory[physical_page_number];
        write_to_disk(virtual_locations[physical_page_number], physical_page_number);
        create_page(physical_page_number, virtual_page_number);
        enqueue(physical_page_number);
        entry.valid = 1;
        entry.exists = 1;
    }
    else if (entry.valid == 0 && entry.exists == 1) {
        int physical_page_number = dequeue();
        entry.physical_page_number = physical_page_number;
        entry.memory_location = physical_memory[physical_page_number];
        write_to_disk(virtual_locations[physical_page_number], entry.physical_page_number);
        write_from_disk(virtual_page_number, entry.physical_page_number);
        enqueue(physical_page_number);
        entry.valid = 1;
    }
    else if (entry.exists == 0 && total_pages < pmpc) {
        entry.physical_page_number = total_pages;
        create_page(total_pages, virtual_page_number);
        enqueue(entry.physical_page_number);
        entry.memory_location = physical_memory[total_pages];
        entry.valid = 1;
        entry.exists = 1;
    }
    uint64_t location = physical_memory[entry.physical_page_number] + offset;
    printf("readbyte: VM location 0x%016llX, which is PM location 0x%016llX, contains value 0x%02lX\n", memory_location, location, *(long *)location);
}

void write_byte() {
    long value = strtol(command_array[2], NULL, 16);
    uint64_t memory_location =  strtol(command_array[1], NULL, 16);
    if (memory_location >= bytes_per_page * vmpc) {
        printf("writebyte: segmentation fault");
        return;
    }
    uint64_t offset = memory_location % bytes_per_page;
    uint64_t virtual_page_number = memory_location / bytes_per_page;
    table_entry entry = page_table[virtual_page_number];
    if (entry.exists == 0 && total_pages >= pmpc) {
        int physical_page_number = dequeue();
        entry.physical_page_number = physical_page_number;
        entry.memory_location = physical_memory[physical_page_number];
        write_to_disk(virtual_locations[physical_page_number], physical_page_number);
        enqueue(physical_page_number);
        create_page(physical_page_number, virtual_page_number);
        entry.valid = 1;
        entry.exists = 1;
    }
    else if (entry.valid == 0 && entry.exists == 1) {
        int physical_page_number = dequeue();
        entry.physical_page_number = physical_page_number;
        entry.memory_location = physical_memory[physical_page_number];
        write_to_disk(virtual_locations[physical_page_number], physical_page_number);
        write_from_disk(virtual_page_number, physical_page_number);
        enqueue(physical_page_number);
        entry.valid = 1;
    }
    else if (entry.exists == 0 && total_pages < pmpc) {
        entry.physical_page_number = total_pages;
        create_page(total_pages, virtual_page_number);
        enqueue(entry.physical_page_number);
        entry.memory_location = physical_memory[total_pages];
        entry.valid = 1;
        entry.exists = 1;
    }
    uint64_t location = physical_memory[entry.physical_page_number] + offset;
    memcpy((void *)location, &value, sizeof(long));
    dirty[virtual_page_number] = 1;
    printf("writebyte: VM location 0x%016llX, which is PM location 0x%016llX, now contains value 0x%02hhX\n", memory_location, location, *(char *)location);
}

void create_page(int physical_page_number_destination, int virtual_page_number) {
    table_entry entry;
    Posix_memalign( &physical_memory[physical_page_number_destination], bytes_per_page, bytes_per_page);
    virtual_locations[physical_page_number_destination] = virtual_page_number;
    entry.memory_location = physical_memory[physical_page_number_destination];
    entry.physical_page_number = physical_page_number_destination;
    entry.valid = 1;
    entry.exists = 1;
    total_pages++;
    page_table[virtual_page_number] = entry;
    printf("physical page at 0x%016llX mapped to virtual page at 0x%016X\n", physical_memory[physical_page_number_destination], virtual_page_number * bytes_per_page);
}

void write_to_disk(int virtual_page_number, int physical_page_number) {
    if (dirty[virtual_page_number] == 1) { //if changed copy to disk
        memcpy((void *)disk[virtual_page_number], (void *)physical_memory[physical_page_number], bytes_per_page);
        printf("physical page at 0x%016llX, which corresponds to virtual page 0x%016X, is evicted and dirty. Copied to disc at 0x%016llX and removed from physical memory\n", physical_memory[physical_page_number], virtual_page_number * bytes_per_page, disk[virtual_page_number]);
        dirty[virtual_page_number] = 0;
    }
    else {
        printf("physical page at 0x%016llX, which corresponds to virtual page 0x%016X, is evicted and not dirty. Removed from physical memory\n", physical_memory[physical_page_number], virtual_page_number * bytes_per_page);
    }
    //set valid bit to 0 now that its on disk
    page_table[virtual_page_number].valid = 0;
}

void write_from_disk(int virtual_page_number_source, int physical_page_number_destination) {
    memcpy((void *)physical_memory[physical_page_number_destination], (void *)disk[virtual_page_number_source], bytes_per_page);
    //set valid bit to 1 now that its in memory
    page_table[virtual_page_number_source].valid = 1;
    printf("physical page at 0x%016llX mapped to virtual page at 0x%016X\n", physical_memory[physical_page_number_destination], virtual_page_number_source * bytes_per_page);
}

void enqueue(int page_number) {
    queue[back_of_queue] = page_number;
    back_of_queue = back_of_queue + 1 % vmpc;
}

int dequeue() {
    int number = queue[front_of_queue];
    front_of_queue = front_of_queue + 1 % vmpc;
    return number;
}

/*
 * ERRORS
 */

void app_error(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void unix_error(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/*
 * WRAPPERS
 */

void Posix_memalign(uint64_t *memptr, size_t alignment, size_t size) {
    int rc;
    rc = posix_memalign((void **)memptr, alignment, size);
    if (rc != 0) {
        app_error("error: posix_memalign failed.");
    }
}

void *Malloc(size_t size) {
    void *p;
    if ((p  = malloc(size)) == NULL) {
        unix_error("Malloc error");
    }
    return p;
}

void *Calloc(size_t nmemb, size_t size) {
    void *p;
    if ((p = calloc(nmemb, size)) == NULL) {
        unix_error("Calloc error");
    }
    return p;
}

void Free(void *ptr) {
    free(ptr);
}