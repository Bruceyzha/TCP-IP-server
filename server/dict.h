typedef struct compress{
    uint8_t* compressed;
    int length;
    int offset;
    int bit_length;
    struct compress* left;
    struct compress* right;
}Compress;


typedef struct Payload{
    int length;
    uint8_t* load;
}payload;

Compress* dict_init();
payload* compress(payload* uncompress);
payload* decompress(payload* compress);