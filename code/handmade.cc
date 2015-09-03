#include "handmade.h"

internal void
render_weird_gradient (game_offscreen_buffer *buffer,
                       int blue_offset, int green_offset)
{
    // TODO(casey): Let's see what the optimizer does

    uint8 *row = (uint8 *)buffer->memory;    
    for(int y = 0;
        y < buffer->height;
        ++y)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = 0;
            x < buffer->width;
            ++x)
        {
            uint8 blue = (x + blue_offset);
            uint8 green = (y + green_offset);

            *pixel++ = ((green << 8) | blue);
        }
        
        row += buffer->pitch;
    }
}

internal void
game_update_render(game_offscreen_buffer *buffer, int blue_offset, int green_offset)
{
  render_weird_gradient(buffer, blue_offset, green_offset);
}
