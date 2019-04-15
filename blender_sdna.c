#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stddef.h>


const char *normalize_type(const char *type)
{
    if(!strcmp(type, "uchar")) return "unsigned char";
    if(!strcmp(type, "short")) return "int16_t";
    if(!strcmp(type, "ushort")) return "uint16_t";
    if(!strcmp(type, "int")) return "int32_t";
    if(!strcmp(type, "long")) return "int64_t";
    if(!strcmp(type, "ulong")) return "uint64_t";
    
    return type;
}

struct blend_block
{
    char block_type[4];
    uint32_t data_size;
    uint64_t struct_pointer;
    uint32_t sdna_idx;
    uint32_t struct_count;
    
};

struct dna_block_strc_field
{
    uint16_t type_idx, name_idx;
};

struct dna_block_strc
{
    uint16_t type_idx;
    uint16_t field_n;
    struct dna_block_strc_field *fields;
};

struct dna_block
{
    char SDNA[4];
    char NAME[4];
    uint32_t names_n;
    char **names;
    char TYPE[4];
    uint32_t types_n;
    char **types;
    char TLEN[4];
    uint16_t *type_lengths; /* type_n times */
    char *STRC[4];
    uint32_t struct_n;
    struct dna_block_strc *strc;
};

int read_dna(void *buf, size_t size)
{
    char *z = buf;
    struct blend_block block;

    z+=12; /* skip header */

check_block:
    memcpy(&block.block_type, z, 4);
/*    printf("block type: %.4s\n", z);*/
    if(!memcmp(block.block_type, "DNA1", 4)) goto dna;
    
    z+=4;
    memcpy(&block.data_size, z, 4);
/*    printf("block size: %d\n", block.data_size);*/
    z+=20; /* end of block header */
    z+=block.data_size; /* go to next block header */
    ptrdiff_t d =  z - (char*)buf;

goto check_block;

dna:
    z+=4; /* skip over block_type */
    memcpy(&block.data_size, z, 4);
    z+=20; /* end of block header */
    
    z+=8; /* skip "SDNA", "NAME" */
    struct dna_block dna;
    memcpy(&dna.names_n, z, 4);
    z+=4;
    
    dna.names = malloc(sizeof(char*) * dna.names_n);
    
    int i, j;
    for(i=0;i < dna.names_n; i++)
    {
        /*printf("%s\n", z);*/
        dna.names[i] = z;
        z+= strlen(z) + 1;
    }
    
    z = (z + 4) - ((intptr_t)z % 4); /* round to multiple of 4 */
    z+=4; /* TYPE */
    memcpy(&dna.types_n, z, 4);
    z+=4;
    
    dna.types = malloc(sizeof(char*) * dna.types_n);
    
/*    printf("dna types:\n");*/
    
    for(i=0; i < dna.types_n; i++)
    {
        /*printf("%s\n", z);*/
        dna.types[i] = z;
        z+= strlen(z) + 1;
    }
    
    z = (z + 4) - ((intptr_t)z % 4); /* round to multiple of 4 */
    z+=4; /* TLEN */
    
    dna.type_lengths = malloc(sizeof(uint16_t) * dna.types_n);
    
    z+= dna.types_n * 2; /* go past type lengths */
    
    z = (z + 4) - ((intptr_t)z % 4); /* round to multiple of 4 */
    z+=4; /* STRC */
    memcpy(&dna.struct_n, z, 4);
    z+=4;    

    struct dna_block_strc *strc = malloc(sizeof(struct dna_block_strc) * dna.struct_n);
    
    for(i=0; i < dna.struct_n; i++)
    {
        memcpy(&strc[i].type_idx, z, 2);
        z+=2;
        memcpy(&strc[i].field_n, z, 2);
        z+=2;
        strc[i].fields = malloc(sizeof(struct dna_block_strc_field) * strc[i].field_n);
        for(j=0; j < strc[i].field_n; j++)
        {
            memcpy(&strc[i].fields[j].type_idx, z, 2);
            z+=2;
            memcpy(&strc[i].fields[j].name_idx, z, 2);
            z+=2;
        }
    }
    /* offset should point to "ENDB" here */
    d =  z - (char*)buf;
    /*printf("%d\n", ofs);*/
    z = (char*)buf + 12; /* back to first block header */
    
    printf("#include <stdint.h>\n#include <inttypes.h>\n\n");
    
    for(i=0; i < dna.struct_n; i++)
    {
        printf("typedef struct %s %s;\n", dna.types[strc[i].type_idx], dna.types[strc[i].type_idx]);
    }
    
    printf("\n\n");
    
    for(i=0; i < dna.struct_n; i++)
    {
        printf("struct %s\n{\n", dna.types[strc[i].type_idx]);

        
        for(j=0; j < strc[i].field_n; j++)
        {
            const char *type, *name;
            uint16_t tlen;
            type = dna.types[strc[i].fields[j].type_idx];
            name = dna.names[strc[i].fields[j].name_idx];
            tlen = dna.type_lengths[strc[i].fields[j].type_idx];


            printf("    %s %s;\n", normalize_type(type), name);
        }
        printf("};\n\n");
    }

    printf("int main() { return 0; }");

    return 0;
}

int main(int argc, char **argv)
{
    if(argc < 1)
    {
        printf("no input file\n");
        return 1;
    }
    
    FILE *bl = fopen(argv[1], "rb");
    if(bl == NULL)
    {
        printf("error opening .blend file\n");
        return 1;
    }
    
    fseek(bl, 0, SEEK_END);
    long siz_bl = ftell(bl);
    rewind(bl);
    void *bl_buf = malloc(siz_bl);
    
    if(bl_buf == NULL)
    {
        printf("out of memory\n");
        return 1;
    }
    
    fread(bl_buf, siz_bl, 1, bl);

    read_dna(bl_buf, siz_bl);

    return 0;
}
