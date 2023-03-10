#include "ccask_file.h"

struct ccask_file {
    FILE* file;
    size_t file_pos;
    uint32_t file_id;
};

ccask_file* ccask_file_init(ccask_file* cf, const char* path, uint32_t fid) {
    return cf;
}

int ccask_file_get(ccask_file* cf, uint32_t value_size, size_t value_pos,
                   uint32_t key_size, uint8_t* buf, size_t bufsz) {
    // decl
    size_t row_size, read_size;
    int ferr;

    row_size = HEADER_BYTES + value_size + key_size;

    //argument error
    if(bufsz < row_size || row_size < HEADER_BYTES ||
            row_size < value_size || row_size < key_size ||
            buf == NULL || cf == NULL)
        return 1;

    if (fseek(cf->file, value_pos, SEEK_SET) != 0) {
        perror("seek error");
        errno = 0;
        return 4;
    }

    read_size = fread(buf, row_size, 1, cf->file);

    if (read_size < 1) {
        printf("read size: %zu\n", read_size);
        for (size_t i = 0; i < read_size*row_size; i++) {
            printf("0x%.02x ", buf[i]);
        }
        puts("");

        ferr = ferror(cf->file);
        printf("ferror: %d\n", ferr);

        if(ferr) perror("fread");

        ferr = feof(cf->file);
        printf("feof: %d\n", ferr);

        ferr = ftell(cf->file);
        printf("ftell: %d\n", ferr);

        printf("file_id: %u\n", cf->file_id);

        return 3;
    }

    return 0;
}
