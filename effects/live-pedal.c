
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <poll.h>
#include <alsa/asoundlib.h>

#include "gain.h"
#include "bitcrush.h"

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define FRAMES 128
#define LATENCY 10000
#define QUEUE_FRAMES 2048

int main(){

    //===========================
    // 1. ALSA HANDLES
    //===========================

    snd_pcm_t *capture_handle;
    snd_pcm_t *playback_handle;


    //===========================
    // 2. AUDIO BUFFER
    //===========================

    int16_t buffer[FRAMES * CHANNELS];

    //store processed frames waiting for playback
    int16_t queue[QUEUE_FRAMES * CHANNELS];

    snd_pcm_uframes_t queued = 0;


    //===========================
    // 3. OPEN AUDIO DEVICES
    //===========================

    //snd_pcm_open() for capture
    int capture_err = snd_pcm_open(
        &capture_handle,
        "hw:2,0",
        SND_PCM_STREAM_CAPTURE,
        SND_PCM_NONBLOCK
    );

    //err checking
    if(capture_err < 0){
        fprintf(stderr, "Capture error: %s\n",
                snd_strerror(capture_err));
        return EXIT_FAILURE;
    }

    //snd_pcm_open() for playback
    int playback_err = snd_pcm_open(
        &playback_handle,
        "hw:2,0",
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_NONBLOCK
    );

    if(playback_err < 0){
        fprintf(stderr, "Playback error: %s\n",
                snd_strerror(playback_err));
        snd_pcm_close(capture_handle);
        return EXIT_FAILURE;
    }


    //===========================
    // 4. CONFIGURE ALSA
    //===========================

    //set sample format: S16_LE

    //set sample rate: 48000

    //set channels: 2

    //set access: interleaved
    int capture_params = snd_pcm_set_params(
        capture_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        0,
        LATENCY
    );

    if(capture_params < 0){
        fprintf(stderr, "Capture configuration error: %s\n",
                snd_strerror(capture_params));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return EXIT_FAILURE;
    }

    int playback_params = snd_pcm_set_params(
        playback_handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        CHANNELS,
        SAMPLE_RATE,
        0,
        LATENCY
    );

    if(playback_params < 0){
        fprintf(stderr, "Playback configuration error: %s\n",
                snd_strerror(playback_params));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return EXIT_FAILURE;
    }

    //start capture explicitly
    int start_err = snd_pcm_start(capture_handle);

    if(start_err < 0){
        fprintf(stderr, "Capture start error: %s\n",
                snd_strerror(start_err));
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return EXIT_FAILURE;
    }

    //get poll descriptors for both devices
    int capture_count = snd_pcm_poll_descriptors_count(
        capture_handle
    );

    int playback_count = snd_pcm_poll_descriptors_count(
        playback_handle
    );

    if(capture_count <= 0 || playback_count <= 0){
        fprintf(stderr, "Invalid poll descriptors\n");
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return EXIT_FAILURE;
    }

    int total_count = capture_count + playback_count;

    struct pollfd *fds = (struct pollfd *)calloc(
        total_count, sizeof(struct pollfd)
    );

    if(fds == NULL){
        fprintf(stderr, "Poll allocation failed\n");
        snd_pcm_close(capture_handle);
        snd_pcm_close(playback_handle);
        return EXIT_FAILURE;
    }

    snd_pcm_poll_descriptors(
        capture_handle, fds, capture_count
    );

    snd_pcm_poll_descriptors(
        playback_handle,
        fds + capture_count,
        playback_count
    );

    printf("Audio started: %d Hz, %d frames\n",
           SAMPLE_RATE, FRAMES);


    //===========================
    // 5. LIVE AUDIO LOOP
    //===========================

    int status = EXIT_SUCCESS;

    while(1){

        //wait for an audio device to become ready
        int ready = poll(fds, total_count, 1000);

        if(ready < 0){
            if(errno == EINTR){
                continue;
            }

            perror("poll");
            status = EXIT_FAILURE;
            break;
        }

        if(ready == 0){
            continue;
        }

        unsigned short capture_events = 0;
        unsigned short playback_events = 0;

        snd_pcm_poll_descriptors_revents(
            capture_handle,
            fds,
            capture_count,
            &capture_events
        );

        snd_pcm_poll_descriptors_revents(
            playback_handle,
            fds + capture_count,
            playback_count,
            &playback_events
        );


        //read frames from guitar
        //snd_pcm_readi()
        if(capture_events & (POLLIN | POLLERR)){

            while(1){

                snd_pcm_sframes_t available = snd_pcm_avail_update(
                    capture_handle
                );

                if(available < 0){
                    int recover = snd_pcm_recover(
                        capture_handle, available, 1
                    );

                    if(recover < 0){
                        fprintf(stderr, "Capture recovery failed: %s\n",
                                snd_strerror(recover));
                        status = EXIT_FAILURE;
                        break;
                    }

                    if(snd_pcm_state(capture_handle) ==
                       SND_PCM_STATE_PREPARED){
                        int restart = snd_pcm_start(capture_handle);

                        if(restart < 0){
                            fprintf(stderr, "Capture restart failed: %s\n",
                                    snd_strerror(restart));
                            status = EXIT_FAILURE;
                            break;
                        }
                    }

                    queued = 0;
                    break;
                }

                if(available == 0){
                    break;
                }

                snd_pcm_uframes_t request =
                    available < FRAMES ? available : FRAMES;

                snd_pcm_sframes_t frames_read = snd_pcm_readi(
                    capture_handle,
                    buffer,
                    request
                );

                if(frames_read == -EAGAIN){
                    break;
                }

                if(frames_read < 0){
                    int recover = snd_pcm_recover(
                        capture_handle, frames_read, 1
                    );

                    if(recover < 0){
                        fprintf(stderr, "Capture recovery failed: %s\n",
                                snd_strerror(recover));
                        status = EXIT_FAILURE;
                        break;
                    }

                    if(snd_pcm_state(capture_handle) ==
                       SND_PCM_STATE_PREPARED){
                        int restart = snd_pcm_start(capture_handle);

                        if(restart < 0){
                            fprintf(stderr, "Capture restart failed: %s\n",
                                    snd_strerror(restart));
                            status = EXIT_FAILURE;
                            break;
                        }
                    }

                    queued = 0;
                    break;
                }

                if(frames_read == 0){
                    break;
                }


                //process samples
                for(int i = 0; i < frames_read; i++){

                    int16_t left = buffer[i * 2];
                    int16_t right = buffer[i * 2 + 1];

                    //===================
                    // EFFECT CHAIN
                    //===================

                    //left = apply_gain(left, 2.0f);
                    //right = apply_gain(right, 2.0f);

                    //left = bitcrush(left, 4096);
                    //right = bitcrush(right, 4096);

                    //store modified samples
                    buffer[i * 2] = left;
                    buffer[i * 2 + 1] = right;
                }

                //make room if playback falls behind
                if(queued + frames_read > QUEUE_FRAMES){
                    snd_pcm_uframes_t drop =
                        queued + frames_read - QUEUE_FRAMES;

                    memmove(
                        queue,
                        queue + drop * CHANNELS,
                        (queued - drop) * CHANNELS *
                            sizeof(int16_t)
                    );

                    queued -= drop;
                }

                //add processed samples to playback queue
                memcpy(
                    queue + queued * CHANNELS,
                    buffer,
                    frames_read * CHANNELS * sizeof(int16_t)
                );

                queued += frames_read;
            }
        }

        if(status == EXIT_FAILURE){
            break;
        }


        //write frames to playback device
        //snd_pcm_writei()
        //
        //dont write unless we actually recieve frames
        if(queued > 0){

            while(queued > 0){

                snd_pcm_sframes_t space = snd_pcm_avail_update(
                    playback_handle
                );

                if(space < 0){
                    int recover = snd_pcm_recover(
                        playback_handle, space, 1
                    );

                    if(recover < 0){
                        fprintf(stderr, "Playback recovery failed: %s\n",
                                snd_strerror(recover));
                        status = EXIT_FAILURE;
                    }

                    break;
                }

                if(space == 0){
                    break;
                }

                snd_pcm_uframes_t request =
                    queued < (snd_pcm_uframes_t)space ?
                    queued : (snd_pcm_uframes_t)space;

                snd_pcm_sframes_t written = snd_pcm_writei(
                    playback_handle,
                    queue,
                    request
                );

                if(written == -EAGAIN){
                    break;
                }

                if(written < 0){
                    int recover = snd_pcm_recover(
                        playback_handle, written, 1
                    );

                    if(recover < 0){
                        fprintf(stderr, "Playback recovery failed: %s\n",
                                snd_strerror(recover));
                        status = EXIT_FAILURE;
                    }

                    break;
                }

                if(written == 0){
                    break;
                }

                queued -= written;

                memmove(
                    queue,
                    queue + written * CHANNELS,
                    queued * CHANNELS * sizeof(int16_t)
                );
            }
        }

        if(status == EXIT_FAILURE){
            break;
        }
    }


    //===========================
    // 6. CLEANUP
    //===========================

    free(fds);

    //snd_pcm_close() capture
    int close_capture = snd_pcm_close(capture_handle);

    //snd_pcm_close() playback
    int close_playback = snd_pcm_close(playback_handle);

    return status;
}

