#include <alsa/asoundlib.h>

#define USB_INTERFACE_NUM_CHANNELS 4

int usb_interface_open_and_initialise(snd_pcm_t ** capture, snd_pcm_t ** playback, const char * dev);
int usb_interface_readi(snd_pcm_t ** capture, char * buffer, unsigned long buffer_frames);
int usb_interface_writei(snd_pcm_t ** capture, char * buffer, unsigned long buffer_frames);
