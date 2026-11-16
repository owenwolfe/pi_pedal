/*============================================================*
    AUTHOR: Owen Wolfe                                        |
    Date: 10/07/2026                                          |
                                                              |
    Hello! I created a .wav file audio writer in pure         |
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

//predecs
void write_le32(FILE *file, uint32_t value);
void write_be32(FILE *file, uint32_t value);

void write_le16(FILE *file, uint16_t value);
void write_be16(FILE *file, uint16_t value);


int main(){

    //open file in binary
    FILE *file = fopen("output.wav", "wb");

    if(file == NULL){
        perror("error opening file");
        return EXIT_FAILURE;
    }

    /*
     *
     * For now these are going to be our
     * audio parameters.
     *
     * These should eventually come from the
     * original wav file that we parsed.
     *
     */

    uint16_t audio_format = 1;
    uint16_t num_channels = 2;
    uint32_t sample_rate = 44100;
    uint16_t bits_per_sample = 16;

    //how many seconds of audio we want
    uint32_t seconds = 5;

    /*
     *
     * Lets calculate the same values that
     * we were reading from the fmt chunk
     * in the parser.
     *
     */

    uint16_t block_align = num_channels * (bits_per_sample / 8);

    uint32_t byte_rate = sample_rate * block_align;

    uint32_t total_frames = sample_rate * seconds;

    uint32_t data_size = total_frames * block_align;

    /*
     *
     * RIFF size does NOT include:
     *
     * "RIFF"       -> 4 bytes
     * RIFF size    -> 4 bytes
     *
     * Everything after those first 8 bytes
     * counts toward RIFF size.
     *
     */

    uint32_t riff_size = 36 + data_size;


    /*
     *
     * Now we are basically going to do
     * everything that the parser did,
     * except backwards.
     *
     */


    //write RIFF
    fwrite("RIFF", 1, 4, file);

    printf("RIFF has been written.\n");
    printf("current offset: %ld\n", ftell(file));


    //write RIFF size
    write_le32(file, riff_size);

    printf("RIFF size: %u\n", riff_size);
    printf("current offset: %ld\n", ftell(file));


    printf("\n");


    /*
     *
     * We just wrote "RIFF" at offset 0-3,
     * and the RIFF size at offset 4-7.
     *
     * Now we should be looking at offset 8.
     *
     */


    //write WAVE
    fwrite("WAVE", 1, 4, file);

    printf("WAVE has came and gone.\n");
    printf("current offset: %ld\n", ftell(file));


    /*
     *
     * Now we need to create our fmt chunk.
     *
     * The next 8 bytes will be:
     *
     * 4 -> chunkID
     * 4 -> chunk size
     *
     */


    printf("\n");

    //begin chunk stats
    printf("Current Offset: %ld\n", ftell(file));


    //write fmt chunk ID
    fwrite("fmt ", 1, 4, file);

    printf("ChunkID: fmt \n");


    /*
     *
     * Standard PCM fmt chunk contents
     * are 16 bytes.
     *
     */

    uint32_t chunk_size = 16;

    write_le32(file, chunk_size);

    printf("Chunk Size: %u\n", chunk_size);


    /*
     *
     * The pointer is now sitting at the
     * first byte of the fmt chunks contents.
     *
     */


    printf("\n");

    printf("Current offset: %ld\n", ftell(file));


    //write audio format
    write_le16(file, audio_format);
    printf("Audio Format: %u\n", audio_format);


    //write number of channels
    write_le16(file, num_channels);
    printf("# of Channels: %u\n", num_channels);


    //write sample rate
    write_le32(file, sample_rate);
    printf("Sample Rate: %u\n", sample_rate);


    //write byte rate
    write_le32(file, byte_rate);
    printf("Byte Rate: %u\n", byte_rate);


    //write block align
    write_le16(file, block_align);
    printf("Block Align: %u\n", block_align);


    //write bits per sample
    write_le16(file, bits_per_sample);
    printf("Bits Per Sample: %u\n", bits_per_sample);



    //====================repeats


    printf("\n");

    //begin next chunk stats
    printf("Current Offset: %ld\n", ftell(file));


    /*
     *
     * Now we are creating the data chunk.
     *
     */


    //write data chunk ID
    fwrite("data", 1, 4, file);

    printf("ChunkID: data\n");


    //write data chunk size
    chunk_size = data_size;

    write_le32(file, chunk_size);

    printf("Chunk Size: %u\n", chunk_size);


    //====================repeats


    /*
     *
     * Now we need to write the PCM frames
     * left and right.
     *
     * Two bytes each, so:
     *
     * 0L 0L 0R 0R 1L 1L 1R 1R
     *
     */


    printf("Current Offset: %ld\n", ftell(file));


    int current_frame = 0;

    while(current_frame < total_frames){

        /*
         *
         * THESE are eventually going to be
         * your processed guitar samples.
         *
         * Right now I am just setting them
         * to zero, which means silence.
         *
         */

        int16_t left = 0;
        int16_t right = 0;


        //write left channel
        write_le16(file, (uint16_t)left);


        //write right channel
        write_le16(file, (uint16_t)right);


        /*
        printf("Left: %d, Right: %d\n", left, right);
        */


        current_frame++;
    }


    printf("All done writing!\n");


    fclose(file);

    return EXIT_SUCCESS;
}


/*===================*/
/*   FUNCTIONS       */
/*===================*/


//write little endian 32 byte
void write_le32(FILE *file, uint32_t value){

    unsigned char bytes[4];

    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;

    fwrite(bytes, 1, 4, file);
}


//write big endian 32 byte
void write_be32(FILE *file, uint32_t value){

    unsigned char bytes[4];

    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;

    fwrite(bytes, 1, 4, file);
}


//write little endian 16 byte
void write_le16(FILE *file, uint16_t value){

    unsigned char bytes[2];

    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;

    fwrite(bytes, 1, 2, file);
}


//write big endian 16 byte
void write_be16(FILE *file, uint16_t value){

    unsigned char bytes[2];

    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;

    fwrite(bytes, 1, 2, file);
}
