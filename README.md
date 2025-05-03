Preface: First github upload, so I'm very new at all of this. Bear with it, any unfortunate souls who drifted this way.


**THE SOFTWARE:**

[Z]ImageScan: A basic touch-friendly interface for navigating a directory of JPG/PNG images (either current working directory or one passed as argument), with buttons for navigating to first/last/previous/next images, plus "fast forwarding" through them quickly. Sorted by filename.

Compile with:

`gcc zimagescan.c `pkg-config --cflags --libs gtk+-3.0` -o zimagescan`

| Key               | Action             |
| ----------------- | ------------------ |
| Left/Right Arrow  | Prev/Next Image    |
| Home/End          | First/Last image   |
| Q                 | Quit               |
| Backspace         | Refresh image list |


**THE SCENARIO:**

I "created" this software in order to use with my Raspberry Pi Zero 2W and Waveshare Zero-DISP-7A touchscreen. I use signag's raspiCamSrv software with an IR camera to capture motion... or, that was the idea, but it just captures everything all the time, necessitating (more or less) a better way to scrub through a large collection of images manually. The better solution would be to actually get motion triggering tuned properly, but this was fun too.

In the end I just put `exec /usr/local/bin/zimagescan path/to/raspiCamSrv/static/events/` into my .xinitrc and called it a day. I now can zip through images to find any action from nocturnal critters as desired.

This use case means that the resolution of 1024x600 is hardcoded in. Being able to pass arguments to specify this would be nice I suppose. It more or less works as a regular window if you're into WMs.


**THE PROCESS:**

The "version 1.0" code was completed in 3 hours, with no prior experience with GUI applications or GTK and no formal training in C, although I have put a few hundred amateurish hours into it with Arduino and RP2040 projects. I primarily guided Copilot in the basic chat web interface, defining my goals and correcting its mistakes, while incrementing the scope slowly. I wanted to be able to tap the image to refresh the image list but I'm too smoothbrained/not motivated enough to figure that out.
