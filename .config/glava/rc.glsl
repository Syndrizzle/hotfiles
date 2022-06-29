
/* The module to use. A module is a set of shaders used to produce
   the visualizer. The structure for a module is the following:
   
   module_name [directory]
       1.frag [file: fragment shader],
       2.frag [file: fragment shader],
       ...
       
   Shaders are loaded in numerical order, starting at '1.frag',
   continuing indefinitely. The results of each shader (except
   for the final pass) is given to the next shader in the list
   as a 2D sampler.
   
   See documentation for more details. */
#request mod radial

/* Window hints */
#request setfloating  false
#request setdecorated true
#request setfocused   false
#request setmaximized false

/* Set window background opacity mode. Possible values are:
   
   "native" - True transparency provided by the compositor. Can
              reduce performance on some systems, depending on
              the compositor used.
   
   "xroot"  - Maintain a copy of the root window's pixmap
              (usually the desktop background) to provide a
              pseudo-transparent effect. Useful when no compositor
              is available or native transparency isn't nessecary.
              Has very little performance impact.
    
   "none"   - Disable window opacity completely. */
#request setopacity "native"

/* Whether to mirror left and right audio input channels from PulseAudio.*/
#request setmirror false

/* OpenGL context and GLSL shader versions, do not change unless
   you *absolutely* know what you are doing. */
#request setversion 3 3
#request setshaderversion 330

/* Window title */
#request settitle "GLava"

/* Window geometry (x, y, width, height) */
#request setgeometry 600 410 720 250

/* Window background color (RGB format).
   Does not work with `setopacity "xroot"` */
#request setbg 1a1b26

/* (X11 only) EWMH Window type. Possible values are:
   
   "desktop", "dock",   "toolbar", "menu",
   "utility", "splash", "dialog",  "normal"
   
   This will set _NET_WM_WINDOW_TYPE to _NET_WM_WINDOW_TYPE_(TYPE),
   where (TYPE) is the one of the window types listed (after being
   converted to uppercase).
   
   Alternatively, you can set this value to "!", which will cause
   the window to be unmanaged. If this is set, then `addxwinstate`
   will do nothing, but you can use "!+" and "!-" to stack on top
   or below other windows.
*/
#request setxwintype "!+"

/* (X11 only) EWMH Window state atoms (multiple can be specified).
   Possible values are:
   
   "modal", "sticky", "maximized_vert", "maximized_horz",
   "shaded", "skip_taskbar", "skip_pager", "hidden", "fullscreen",
   "above", "below", "demands_attention", "focused", "pinned"
   
   This will add _NET_WM_STATE_(TYPE) atoms to _NET_WM_STATE,
   where (TYPE) is one of the window states listed (after being
   converted to uppercase).
   
   The lines below (commented out by default) are of relevance
   if you are trying to get GLava to behave as a desktop widget
   and your WM is not correctly responding to the "desktop" value
   for `setxwintype`.
*/
// #request addxwinstate "sticky"
// #request addxwinstate "skip_taskbar"
// #request addxwinstate "skip_pager"
// #request addxwinstate "above"
// #request addxwinstate "pinned"

/* (X11 only) Use the XShape extension to support clicking through
   the GLava window. Useful when you want to interact with other
   desktop windows (icons, menus, desktop shells). Enabled by
   default when GLava itself is a desktop window. */
#request setclickthrough false

/* Audio source

   When the "pulseaudio" backend is set, this can be a number or
   a name of an audio sink or device to record from. Set to "auto"
   to use the default output device.
   
   When the "fifo" backend is set, "auto" is interpreted as
   "/tmp/mpd.fifo". Otherwise, a valid path should be provided. */
#request setsource "auto"

/* Buffer swap interval (vsync), set to '0' to prevent
   waiting for refresh, '1' (or more) to wait for the specified
   amount of frames. */
#request setswap 1

/* Linear interpolation for audio data frames. Drastically
   improves smoothness with configurations that yield low UPS
   (`setsamplerate` and `setsamplesize`), or monitors that have
   high refresh rates.
   
   This feature itself, however, will effect performance as it
   will have to interpolate data every frame on the CPU. It will
   automatically (and temporarily) disable itself if the update
   rate is close to, or higher than the framerate:
   
   if (update_rate / frame_rate > 0.9) disable_interpolation;
   
   This will delay data output by one update frame, so it can
   desync audio with visual effects on low UPS configs. */
#request setinterpolate true

/* Frame limiter, set to the frames per second (FPS) desired or
   simply set to zero (or lower) to disable the frame limiter. */
#request setframerate 0

/* Suspends rendering if a fullscreen window is focused while
   GLava is still visible (ie. on another monitor). This prevents
   rendering from interfering with other graphically intensive
   tasks.

   If GLava is minimized or completely obscured, it will not
   render regardless of this option. */
#request setfullscreencheck false

/* Enable/disable printing framerate every second. 'FPS' stands
   for 'Frames Per Second', and 'UPS' stands for 'Updates Per
   Second'. Updates are performed when new data is submitted
   by pulseaudio, and require transformations to be re-applied
   (thus being a good measure of how much work your CPU has to
   perform over time) */
#request setprintframes true

/* PulseAudio sample buffer size. Lower values result in more
   frequent audio updates (also depends on sampling rate), but
   will also require all transformations to be applied much 
   more frequently (CPU intensive).
   
   High (>2048, with 22050 Hz) values will decrease accuracy
   (as some signals can be missed by transformations like FFT)
   
   The following settings (@22050 Hz) produce the listed rates: 
   
   Sample    UPS                  Description
   - 2048 -> 43.0  (low accuracy, cheap), use with < 60 FPS
   - 1024 -> 86.1  (high accuracy, expensive), use with >= 60 FPS
   -  512 -> 172.3 (extreme accuracy, very expensive), use only
                   for graphing accurate spectrum data with
                   custom modules.
   
   If the framerate drops below the update rate, the update rate
   will be locked to the framerate (to prevent wasting CPU time).
   This behaviour means you can use a 1024 sample size on a 60Hz
   monitor with vsync enabled to get 60FPS and 60UPS.
   
   For high refresh rate monitors (120+ Hz), it's recommended to
   also stick with the 1024 sample size and use interpolation to
   smooth the data, as accuracy beyond this setting is mostly
   meaningless for visual purposes.
*/
#request setsamplesize 1024

/* Audio buffer size to be used for processing and shaders. 
   Increasing this value can have the effect of adding 'gravity'
   to FFT output, as the audio signal will remain in the buffer
   longer.

   This value has a _massive_ effect on FFT performance and
   quality for some modules. */
#request setbufsize 4096

/* PulseAudio sample rate. Lower values can add 'gravity' to
   FFT output, but can also reduce accuracy. Most hardware
   samples at 44100Hz.
   
   Lower sample rates also can make output more choppy, when
   not using interpolation. It's generally OK to leave this
   value unless you have a strange PulseAudio configuration.

   This option does nothing when using the "fifo" audio
   backend. Instead, an ideal rate should be be configured
   in the application generating the output. */
#request setsamplerate 22050

/*                    ** DEPRECATED **
   Force window geometry (locking the window in place), useful
   for some pesky WMs that try to reposition the window when
   embedding in the desktop.
   
   This routinely sends X11 events and should be avoided. */
#request setforcegeometry false

/*                    ** DEPRECATED **
   Force window to be raised (focused in some WMs), useful for
   WMs that have their own stacking order for desktop windows.
   
   This routinely sends X11 events and should be avoided. */
#request setforceraised false

/*                    ** DEPRECATED **
   Scale down the audio buffer before any operations are 
   performed on the data. Higher values are faster.
   
   This value can affect the output of various transformations,
   since it applies (crude) averaging to the data when shrinking
   the buffer. It is reccommended to use `setsamplerate` and
   `setsamplesize` to improve performance or accuracy instead. */
#request setbufscale 1
