#if !defined(HANDMADE_H)

struct game_offscreen_buffer
{
    // NOTE(casey): Pixels are always 32-bits wide, Memory Order BB GG RR XX
    void *memory;
    int width;
    int height;
    int pitch;
};

internal void game_update_render (game_offscreen_buffer *buffer,
                                  int blue_offset, int green_offset);

#define HANDMADE_H
#endif
