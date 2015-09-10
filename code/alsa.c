/* This small demo sends a simple sinusoidal wave to your speakers.  */
/* See http://alsamodular.sourceforge.net/alsa_programming_howto.html */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>

// typedef int (*transfer_loop_fn) (snd_pcm_t * handle, signed short *samples,
// snd_pcm_channel_area_t * areas); 
typedef struct _sound_info_t sound_info_t;
struct _sound_info_t {
    char *device;		/* playback device */
    snd_pcm_format_t format;	/* sample format */
    snd_pcm_stream_t stream;	/* stream type */
    snd_pcm_access_t access;	/* access type */
    unsigned int rate;	/* stream rate */
    unsigned int channel_count;	/* count of channels */
    unsigned int buffer_time;	/* ring buffer length in us */
    unsigned int period_time;	/* period time in us */
    double tone_hz;		/* sinusoidal wave frequency in Hz */
    int resample;		/* enable alsa-lib resampling */
    int period_event = 0;	/* produce poll event after each period */
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    snd_output_t *output;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    // transfer_loop_fn transfer_loop;
};

typedef struct _sound_buffer_t sound_buffer_t;
struct _sound_buffer_t {
    signed short *samples;
    snd_pcm_channel_area_t *areas;
};

static void
generate_sine (sound_buffer_t * sound_buffer, sound_info_t * sound_info,
               snd_pcm_uframes_t offset, double *_phase, error_t ** error) {
    static double max_phase = 2.0 * M_PI;
    double phase = *_phase;
    double step = max_phase * sound_info->tone_hz / (double) sound_info->rate;
    unsigned char *samples[sound_info->channel_count];
    int steps[sound_info->channel_count];
    int bits_per_sample = snd_pcm_format_width (sound_info->format);
    unsigned int maxval = (1 << (bits_per_sample - 1)) - 1;
    int bytes_per_sample = bits_per_sample / 8;	/* bytes per sample */
    int phys_bytes_per_sample = snd_pcm_format_physical_width (sound_info->format) / 8;
    int big_endian = snd_pcm_format_big_endian (sound_info->format) == 1;
    int to_unsigned = snd_pcm_format_unsigned (sound_info->format) == 1;
    int is_float = (sound_info->format == SND_PCM_FORMAT_FLOAT_LE
                    || sound_info->format == SND_PCM_FORMAT_FLOAT_BE);

    error_return_if (is_float,, "float format not supported");

    /* verify and prepare the contents of areas */
    for (unsigned int channel_index = 0; channel_index < sound_info->channel_count;
         channel_index++) {
        error_return_if ((sound_buffer->areas[channel_index].first % 8) != 0,,
                         "areas[%i].first == %i, aborting...", channel_index,
                         areas[channel_index].first);
        samples[channel_index] =
            /* (signed short *) */
            (((unsigned char *) areas[channel_index].addr) +
             (areas[channel_index].first / 8));
        error_return_if ((areas[channel_index].step % 16) != 0,,
                         "areas[%i].step == %i, aborting...\n", channel_index,
                         areas[channel_index].step);
        steps[channel_index] = areas[channel_index].step / 8;
        samples[channel_index] += offset * steps[channel_index];
    }
    /* fill the channel areas */
    int count = sound_info->period_size;
    while (count-- > 0) {
        int val = sin (phase) * maxval;
        if (to_unsigned) {
            val ^= 1U << (bits_per_sample - 1);
        }

        for (unsigned int channel_index = 0; channel_index < sound_info->channel_count;
             channel_index++) {
            /* Generate data in native endian format */
            if (big_endian) {
                for (int i = 0; i < bytes_per_sample; i++)
                    *(samples[channel_index] + phys_bytes_per_sample - 1 - i) =
                        (val >> i * 8) & 0xff;
            }
            else {
                for (int i = 0; i < bytes_per_sample; i++)
                    *(samples[channel_index] + i) = (val >> i * 8) & 0xff;
            }
            samples[channel_index] += steps[channel_index];
        }
        phase += step;
        if (phase >= max_phase)
            phase -= max_phase;
    }
    *_phase = phase;
}

static void
set_hw_params (sound_info_t * sound_info, error_t ** error) {
    int res;

    snd_pcm_hw_params_alloca (&sound_info->hw_params);

    /* initialize hw_params with the full configuration space of the soundcard */
    res = snd_pcm_hw_params_any (sound_info->handle, sound_info->hw_params);
    error_return_if (res < 0,,
                     "broken configuration for playback: no configurations available: %s\n",
                     snd_strerror (res));

    /* set the resampling rate */
    res = snd_pcm_hw_params_set_rate_resample (sound_info->handle,
                                               sound_info->hw_params, sound_info->resample);
    error_return_if (res < 0,, "resampling setup failed for playback: %s\n",
                     snd_strerror (res));

    /* set the interleaved read/write format */
    res = snd_pcm_hw_params_set_access (sound_info->handle, sound_info->hw_params,
                                        sound_info->access);
    error_return_if (res < 0,, "access type not available for playback: %s\n",
                     snd_strerror (res));

    /* set the sample format */
    res = snd_pcm_hw_params_set_format (sound_info->handle, sound_info->hw_params,
                                        sound_info->format);
    error_return_if (res < 0,, "sample format not available for playback: %s\n",
                     snd_strerror (res));

    /* set the count of channels */
    res = snd_pcm_hw_params_set_channels (sound_info->handle, sound_info->hw_params,
                                          sound_info->channel_count);
    error_return_if (res < 0,,
                     "channels count (%i) not available for playbacks: %s\n",
                     sound_info->channel_count, snd_strerror (res));

    /* set the stream rate */
    unsigned int exact_rate = sound_info->sampling_rate;
    res = snd_pcm_hw_params_set_rate_near (sound_info->handle, sound_info->hw_params,
                                           &exact_rate, 0);
    error_return_if (res < 0,, "rate %iHz not available for playback: %s\n",
                     sound_info->sampling_rate, snd_strerror (res));
    error_return_if (exact_rate != sound_info->sampling_rate,,
                     "rate doesn't match (requested %iHz, get %iHz)\n",
                     sound_info->sampling_rate, exact_rate);

    /* set the buffer time */
    res = snd_pcm_hw_params_set_buffer_time_near (sound_info->handle,
                                                  sound_info->hw_params,
                                                  &sound_info->buffer_time, &dir);
    error_return_if (res < 0,, "unable to set buffer time %i for playback: %s\n",
                     sound_info->buffer_time, snd_strerror (res));

    snd_pcm_uframes_t size;
    res = snd_pcm_hw_params_get_buffer_size (sound_info->hw_params, &size);
    error_return_if (res < 0,, "unable to get buffer size for playback: %s\n",
                     snd_strerror (res));
    sound_info->buffer_size = size;

    /* set the period time */
    int dir;
    res = snd_pcm_hw_params_set_period_time_near (sound_info->handle,
                                                  sound_info->hw_params,
                                                  &sound_info->period_time, &dir);
    error_return_if (res < 0,, "unable to set period time %i for playback: %s\n",
                     sound_info->period_time, snd_strerror (res));
    res = snd_pcm_hw_params_get_period_size (sound_info->hw_params, &size, &dir);
    error_return_if (res < 0,, "unable to get period size for playback: %s\n",
                     snd_strerror (res));
    sound_info->period_size = size;

    /* write the parameters to device */
    res = snd_pcm_hw_params (sound_info->handle, sound_info->hw_params);
    error_return_if (res < 0,, "unable to set hw params for playback: %s\n",
                     snd_strerror (res));
}

static void
set_sw_params (sound_info_t * sound_info, error_t ** error) {
    int res;

    snd_pcm_sw_params_alloca (&sound_info->sw_params);

    /* get the current sw_params */
    res = snd_pcm_sw_params_current (sound_info->handle, sound_info->sw_params);
    error_return_if (res < 0,,
                     "unable to determine current sw_params for playback: %s\n",
                     snd_strerror (res));

    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    res = snd_pcm_sw_params_set_start_threshold (sound_info->handle,
                                                 sound_info->sw_params,
                                                 (sound_info->buffer_size /
                                                  sound_info->period_size) *
                                                 sound_info->period_size);
    error_return_if (res < 0,,
                     "unable to set start threshold mode for playback: %s\n",
                     snd_strerror (res));

    /* allow the transfer when at least period_size samples can be processed */
    /* or disable this mechanism when period event is enabled (aka intresupt like style
       processing) */
    res = snd_pcm_sw_params_set_avail_min (sound_info->handle, sound_info->sw_params,
                                           sound_info->period_event ? sound_info->
                                           buffer_size : sound_info->period_size);
    error_return_if (res < 0,, "unable to set avail min for playback: %s\n",
                     snd_strerror (res));

    /* enable period events when requested */
    if (sound_info->period_event) {
        res = snd_pcm_sw_params_set_period_event (sound_info->handle,
                                                  sound_info->sw_params, 1);
        error_return_if (res < 0,, "unable to set period event: %s\n", snd_strerror (res));
    }

    /* write the parameters to the playback device */
    res = snd_pcm_sw_params (sound_info->handle, sound_info->sw_params);
    error_return_if (res < 0,, "unable to set sw params for playback: %s\n",
                     snd_strerror (res));
}

/* Underrun and suspend recovery */

static int
xrun_recovery (snd_pcm_t * handle, int err, error_t ** error) {
    printf ("stream recovery\n");

    if (err == -EPIPE) {	/* under-run */
        err = snd_pcm_prepare (handle);
        if (err < 0) {
            printf ("Can't recovery from underrun, prepare failed: %s\n",
                    snd_strerror (err));
        }
        `return 0;
    }
    else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume (handle)) == -EAGAIN) {
            sleep (1);	/* wait until the suspend flag is released */
        }
        if (err < 0) {
            err = snd_pcm_prepare (handle);
            if (err < 0) {
                printf ("Can't recovery from suspend, prepare failed: %s\n",
                        snd_strerror (err));
            }
        }
        return 0;
    }
    return err;
}

/* Transfer method - write only */

static void
sound_buffer_play (sound_buffer_t * sound_buffer, sound_info_t * sound_info, error_t ** error) {
    error_t *err = NULL;
    int res;

    while (1) {
        double phase = 0;
        generate_sine (sound_buffer, sound_info, 0, &phase, &err);
        error_return_if (err != NULL, err, "error generating sine");
        signed short *ptr = sound_buffer->samples;
        int cptr = sound_info->period_size;
        while (cptr > 0) {
            res = snd_pcm_writei (sound_info->handle, ptr, cptr);
            if (res == -EAGAIN) {
                continue;
            }
            if (res < 0) {
                xrun_recovery (sound_info->handle, res, &err);
                error_return_if (err != NULL,, "write error: %s\n",
                                 snd_strerror (err));
                break;	/* skip one period */
            }
            ptr += res * sound_info->channel_count;
            cptr -= res;
        }
    }
}

static void
sound_info_init (sound_info_t * sound_info, error_t ** error) {
    int res;
    error_t *err = NULL;

    res = snd_output_stdio_attach (&sound_info->output, stdout, 0);
    error_return_if (res < 0,, "output failed: %s", snd_strerror (res));

    /* opens a PCM. we use the playback stream (as opposed to the capture stream 0, the
       standard opening mode */
    res = snd_pcm_open (&sound_info->handle, sound_info->device, sound_info->stream, 0);
    error_return_if (res < 0,, "could not open PCM device: %s", sndstrerror (res));

    set_hw_params (&sound_info, &err);
    error_goto_if (err != NULL, pcm_close, err, "could not set hw params");

    set_sw_params (&sound_info, &err);
    error_goto_if (err != NULL, pcm_close, err, "could not set sw params");

pcm_close:
    snd_pcm_close (sound_info->handle);
}

void
sound_buffer_init (sound_buffer_t * sound_buffer, sound_info_t * sound_info, error_t ** error) {
    sound_buffer->samples =
        malloc ((sound_info->period_size * sound_info->channel_count *
                 snd_pcm_format_physical_width (sound_info->format)) / 8);
    error_return_if (sound_buffer->samples == NULL,, "no enough memory");

    sound_buffer->areas = calloc (sound_info->channel_count, sizeof (snd_pcm_channel_area_t));
    error_return_if (sound_buffer->areas == NULL,, "no enough memory\n");

    for (unsigned int i = 0; i < sound_buffer->channel_count; i++) {
        sound_buffer->areas[i].addr = sound_buffer->samples;
        sound_buffer->areas[i].first =
            i * snd_pcm_format_physical_width (sound_info->format);
        sound_buffer->areas[i].step =
            channels * snd_pcm_format_physical_width (sound_info->format);
    }
}

void
sound_buffer_free (sound_buffer_t * sound_buffer) {
    free (sound_buffer->samples);
    free (sound_buffer->areas);
}

int
main (int argc, char *argv[]) {
    int res;
    error_t *err = NULL;

    char pcm_device[] = "plughw:0,0";

    sound_info_t sound_info = { };
    sound_info.device = pcm_device;
    sound_info.format = SND_PCM_FORMAT_S16;	/* Signed 16bit CPU endian */
    sound_info.stream = SND_PCM_STREAM_PLAYBACK;	/* playback stream */
    sound_info.access = SND_PCM_ACCESS_RW_INTERLEAVED	/* access type */
        sound_info.sampling_rate = 44100;	/* stream rate */
    sound_info.channel_count = 2;	/* count of channels */
    sound_info.buffer_time = 500000;	/* ring buffer length in us */
    sound_info.period_time = 100000;	/* period time in us */
    sound_info.tone_hz = 440.0f;	/* sinusoidal wave frequency in Hz */
    sound_info.resampling = 1;	/* enable alsa-lib resampling */
    sound_info.period_event = 0;	/* produce poll event after each period */

    sound_info_init (&sound_info, &err);
    error_log_if (err != NULL,, "unable to initialize ALSA sound info");

    sound_buffer_init (&sound_buffer, &sound_info, &err);
    error_log_if (err != NULL,, "unable to initialize ALSA sound buffer");

    sound_buffer_play (&sound_buffer, &sound_info, &err);
    error_log_if (err != NULL,, "unable to play ALSA sound buffer");

buffer_cleanup:
    sound_buffer_free (&sound_buffer);

pcm_close:
    snd_pcm_close (sound_info->handle);

    return 0;
}
