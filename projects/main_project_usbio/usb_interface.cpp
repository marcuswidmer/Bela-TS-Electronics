#include "usb_interface.hpp"

#include <stdio.h>
#include <stdlib.h>

int usb_interface_open_and_initialise(snd_pcm_t ** capture, snd_pcm_t ** playback, const char * dev)
{
    int err;
    int buffer_frames = 16;
    unsigned int rate = 44100;
  
    snd_pcm_hw_params_t *hw_params_capture;
    snd_pcm_hw_params_t *hw_params_playback;
    snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE;

	if ((err = snd_pcm_open (capture, dev, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        printf("cannot open audio capture device %s (%s)\n", 
                 dev,
                 snd_strerror (err));
        return -1;
    }

    printf("audio interface capture opened\n");
           
    if ((err = snd_pcm_hw_params_malloc (&hw_params_capture)) < 0) {
        printf("cannot allocate hardware parameter structure (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params_capture capture allocated\n");
                 
    if ((err = snd_pcm_hw_params_any (*capture, hw_params_capture)) < 0) {
        printf("cannot initialize hardware parameter structure (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params capture initialized\n");

    if ((err = snd_pcm_hw_params_set_access (*capture, hw_params_capture, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("cannot set access type (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params access capture set\n");

    if ((err = snd_pcm_hw_params_set_format (*capture, hw_params_capture, format)) < 0) {
        printf("cannot set sample format (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params format capture set\n");

    if ((err = snd_pcm_hw_params_set_rate_near (*capture, hw_params_capture, &rate, 0)) < 0) {
        printf("cannot set sample rate (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params rate capture set\n");

    if ((err = snd_pcm_hw_params_set_channels (*capture, hw_params_capture, USB_INTERFACE_NUM_CHANNELS)) < 0) {
        printf("cannot set channel count (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params channels capture set\n");

	if ((err = snd_pcm_hw_params_set_buffer_size(*capture, hw_params_capture, buffer_frames)) < 0) {
        printf("cannot set buffer size (%s)\n",
                 snd_strerror (err));
        exit (1);
    }

    printf("hw_params buffer size capture set\n");

    if ((err = snd_pcm_hw_params (*capture, hw_params_capture)) < 0) {
        printf("cannot set parameters (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params capture set\n");

    snd_pcm_hw_params_free (hw_params_capture);

    printf("hw_params capture freed\n");

    if ((err = snd_pcm_prepare (*capture)) < 0) {
        printf("cannot prepare audio interface capture for use (%s)\n",
                 snd_strerror (err));
        return -1;
    }


    printf("audio interface capture prepared\n");

    if ((err = snd_pcm_open (playback, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("cannot open audio playback device %s (%s)\n", 
                 dev,
                 snd_strerror (err));
    return -1;
    }

    printf("audio interface playback opened\n");
       
    if ((err = snd_pcm_hw_params_malloc (&hw_params_playback)) < 0) {
        printf("cannot allocate hardware parameter structure (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params_playback playback allocated\n");
         
    if ((err = snd_pcm_hw_params_any (*playback, hw_params_playback)) < 0) {
        printf("cannot initialize hardware parameter structure (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params playback initialized\n");

    if ((err = snd_pcm_hw_params_set_access (*playback, hw_params_playback, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        printf("cannot set access type (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params access playback set\n");

    if ((err = snd_pcm_hw_params_set_format (*playback, hw_params_playback, format)) < 0) {
        printf("cannot set playback sample format (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params format playback set\n");

    if ((err = snd_pcm_hw_params_set_rate_near (*playback, hw_params_playback, &rate, 0)) < 0) {
        printf("cannot set playback sample rate (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params rate playback set\n");

    if ((err = snd_pcm_hw_params_set_channels (*playback, hw_params_playback, USB_INTERFACE_NUM_CHANNELS)) < 0) {
        printf("cannot set playback channel count (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params channels playback set\n");

  	if ((err = snd_pcm_hw_params_set_buffer_size(*playback, hw_params_playback, buffer_frames)) < 0) {
        printf("cannot set buffer size (%s)\n",
                 snd_strerror (err));
        exit (1);
    }

    printf("hw_params buffer size playback set\n");

    if ((err = snd_pcm_hw_params (*playback, hw_params_playback)) < 0) {
        printf("cannot set parameters (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("hw_params playback set\n");

    snd_pcm_hw_params_free (hw_params_playback);

    printf("hw_params playback freed\n");

    if ((err = snd_pcm_prepare (*playback)) < 0) {
        printf("cannot prepare audio interface playback for use (%s)\n",
                 snd_strerror (err));
        return -1;
    }

    printf("audio interface playback prepared\n");

    return 0;
}

int usb_interface_readi(snd_pcm_t ** capture, char * buffer, unsigned long buffer_frames)
{
	int err = snd_pcm_readi(*capture, buffer, buffer_frames);
    
    if (err != buffer_frames) {
    	printf ("read from audio interface failed (%d, %s)\n",
        	err, snd_strerror (err));
        return -1;
    }

    return 0;
}

int usb_interface_writei(snd_pcm_t ** playback, char * buffer, unsigned long buffer_frames)
{
	int err = snd_pcm_writei(*playback, buffer, buffer_frames);
    
    if (err != buffer_frames) {
    	printf ("write to audio interface failed (%d, %s)\n",
        	err, snd_strerror (err));
        return -1;
    }

    return 0;
}
