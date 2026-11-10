/*============================================================*
    AUTHOR: Owen Wolfe                                        |
    Date: 08/16/2026                                          |
                                                              |
    Hello! I created a .wav file audio parser in pure         |
    C to do modulation in the future. The parameters are      |
    very specialized to my specific guitar audio, so this may |
    not work for all applications.                            |
                                                              |
    Thanks!                                                   |
==============================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gain.h"
//#include "bitcrush.h"


//predecs
uint32_t read_le32(const unsigned char bytes[4]);
uint32_t read_be32(const unsigned char bytes[4]);

uint16_t read_le16(const unsigned char bytes[2]);
uint16_t read_be16(const unsigned char bytes[2]);

void write_le32(FILE *file, uint32_t value);
void write_le16(FILE *file, uint16_t value);


int main(){

    //open input file in binary
    FILE *file = fopen("test.wav", "rb");

    if(file == NULL){
        perror("error opening file");
        return EXIT_FAILURE;
    }


    //open output file in binary
    FILE *output_file = fopen("output.wav", "wb");

    if(output_file == NULL){
        perror("error opening output file");
        fclose(file);
        return EXIT_FAILURE;
    }


    //buffers for both 4 and 2 bytes
    unsigned char buffer4[4];
    unsigned char buffer2[2];


    //lets check to see what we got
    size_t result = fread(buffer4, 1, sizeof(buffer4), file);

    if(memcmp(buffer4, "RIFF", 4) == 0){
        printf("RIFF size begins at position: %ld\n", ftell(file));
    }
    else{
        for(int i = 0; i < sizeof(buffer4); i++){
            printf("%02x", buffer4[i]);
        }
    }


    //write RIFF to output
    fwrite("RIFF", 1, 4, output_file);


    //read in RIFF chunk size
    result = fread(buffer4, 1, sizeof(buffer4), file);

    uint32_t riff_size = read_le32(buffer4);

    printf("RIFF size: %u\n", riff_size);
    printf("current offset: %ld\n", ftell(file));


    //write RIFF size to output
    write_le32(output_file, riff_size);


    printf("\n");


    /*
     *
     * We just found "RIFF" at offset 0-3,
     * and the RIFF size at offset 4-7.
     *
     * Now we should be looking at offset 8.
     *
     */


    result = fread(buffer4, 1, sizeof(buffer4), file);


    if(memcmp(buffer4, "WAVE", 4) == 0){
        printf("WAVE has came and gone.\n");
    }


    //write WAVE to output
    fwrite("WAVE", 1, 4, output_file);


    printf("current offset: %ld\n", ftell(file));
    printf("\n");


    //begin chunk stats
    printf("Current Offset: %ld\n", ftell(file));


    result = fread(buffer4, 1, sizeof(buffer4), file);


    printf("ChunkID: %c%c%c%c\n",
            buffer4[0],
            buffer4[1],
            buffer4[2],
            buffer4[3]);


    //write fmt chunk id
    fwrite(buffer4, 1, 4, output_file);


    result = fread(buffer4, 1, sizeof(buffer4), file);


    uint32_t chunk_size = read_le32(buffer4);

    printf("Chunk Size: %u\n", chunk_size);


    //write fmt chunk size
    write_le32(output_file, chunk_size);


    printf("\n");

    printf("Current offset: %ld\n", ftell(file));


    result = fread(buffer2, 1, sizeof(buffer2), file);

    uint16_t audio_format = read_le16(buffer2);

    printf("Audio Format: %u\n", audio_format);

    write_le16(output_file, audio_format);


    result = fread(buffer2, 1, sizeof(buffer2), file);

    uint16_t num_channels = read_le16(buffer2);

    printf("# of Channels: %u\n", num_channels);

    write_le16(output_file, num_channels);


    result = fread(buffer4, 1, sizeof(buffer4), file);

    uint32_t sample_rate = read_le32(buffer4);

    printf("Sample Rate: %u\n", sample_rate);

    write_le32(output_file, sample_rate);


    result = fread(buffer4, 1, sizeof(buffer4), file);

    uint32_t byte_rate = read_le32(buffer4);

    printf("Byte Rate: %u\n", byte_rate);

    write_le32(output_file, byte_rate);


    result = fread(buffer2, 1, sizeof(buffer2), file);

    uint16_t block_align = read_le16(buffer2);

    printf("Block Align: %u\n", block_align);

    write_le16(output_file, block_align);


    result = fread(buffer2, 1, sizeof(buffer2), file);

    uint16_t bits_per_sample = read_le16(buffer2);

    printf("Bits Per Sample: %u\n", bits_per_sample);

    write_le16(output_file, bits_per_sample);


    //====================repeats


    printf("\n");

    printf("Current Offset: %ld\n", ftell(file));


    result = fread(buffer4, 1, sizeof(buffer4), file);


    printf("ChunkID: %c%c%c%c\n",
            buffer4[0],
            buffer4[1],
            buffer4[2],
            buffer4[3]);


    //write data chunk id
    fwrite(buffer4, 1, 4, output_file);


    result = fread(buffer4, 1, sizeof(buffer4), file);


    chunk_size = read_le32(buffer4);

    printf("Chunk Size: %u\n", chunk_size);


    //write data chunk size
    write_le32(output_file, chunk_size);


    //====================repeats


    /*
     *
     * PCM frames:
     *
     * 0L 0L 0R 0R
     * 1L 1L 1R 1R
     *
     */


    printf("Current Offset: %ld\n", ftell(file));


    int current_frame = 0;
    int total_frames = chunk_size / block_align;

    int changed_frames = 0;


    while(current_frame < total_frames){

        result = fread(buffer2, 1, sizeof(buffer2), file);
        int16_t left = (int16_t)read_le16(buffer2);

        result = fread(buffer2, 1, sizeof(buffer2), file);
        int16_t right = (int16_t)read_le16(buffer2);


        //save original samples
        int16_t old_left = left;
        int16_t old_right = right;


        /*
         * ==============================
         *       EFFECT CHAIN
         * ==============================
         *
         * Add/remove effects here.
         *
         * They run from top to bottom.
         *
         */


        //GAIN
        left = apply_gain(left, 8.0);
        right = apply_gain(right, 8.0);


        //BITCRUSH
        //left = bitcrush(left, 4096);
        //right = bitcrush(right, 4096);


        /*
         * ==============================
         *     END EFFECT CHAIN
         * ==============================
         */


        if(left != old_left || right != old_right){
            changed_frames++;
        }


        //write modified audio
        write_le16(output_file, (uint16_t)left);
        write_le16(output_file, (uint16_t)right);


        current_frame++;
    }


    printf("\n");

    printf("All done parsing and processing!\n");
    printf("Total Frames: %d\n", total_frames);
    printf("Changed Frames: %d\n", changed_frames);


    if(changed_frames > 0){
        printf("AUDIO DATA WAS CHANGED.\n");
    }
    else{
        printf("AUDIO DATA WAS NOT CHANGED.\n");
    }


    fclose(file);
    fclose(output_file);

    return EXIT_SUCCESS;
}


/*===================*/
/*   FUNCTIONS       */
/*===================*/


//read little endian 32 bit
uint32_t read_le32(const unsigned char bytes[4]){

    return (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
}


//read big endian 32 bit
uint32_t read_be32(const unsigned char bytes[4]){

    return ((uint32_t)bytes[0] << 24)
        | ((uint32_t)bytes[1] << 16)
        | ((uint32_t)bytes[2] << 8)
        | (uint32_t)bytes[3];
}


//read little endian 16 bit
uint16_t read_le16(const unsigned char bytes[2]){

    return (uint16_t)bytes[0]
        | ((uint16_t)bytes[1] << 8);
}


//read big endian 16 bit
uint16_t read_be16(const unsigned char bytes[2]){

    return ((uint16_t)bytes[0] << 8)
        | ((uint16_t)bytes[1]);
}


//write little endian 32 bit
void write_le32(FILE *file, uint32_t value){

    unsigned char bytes[4];

    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;

    fwrite(bytes, 1, 4, file);
}


//write little endian 16 bit
void write_le16(FILE *file, uint16_t value){

    unsigned char bytes[2];

    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;

    fwrite(bytes, 1, 2, file);
}
