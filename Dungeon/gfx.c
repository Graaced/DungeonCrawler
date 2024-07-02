#include "dungeon.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define NORTH 8
#define SOUTH 4
#define WEST 2
#define EAST 1

typedef struct DungeonGlyph
{

    SDL_Texture *texture;
    SDL_Rect rect;
    int advance;

} DungeonGlyph;

DungeonGlyph glyphs[95];
float scale = 1.0f;
float camera_x = 0.0f;
float camera_y = 0.0f;
int draw_tile_counter = 0;

int draw_room(SDL_Renderer* renderer, SDL_Texture* texture, const float room_x, const float room_y, const unsigned char bitmask)
{
    const int room_tile_width = 8;
    const int room_tile_height = 8;

    for(int y = 0; y < room_tile_height; y++)
    {
        for(int x = 0; x < room_tile_width; x++)
        {
            draw_tile(renderer, texture, 9, 7, room_x + x * 16 * scale, room_y + y * 16 * scale);
        }
    }

    if(bitmask & NORTH)
    {
        for(int x = 0; x < room_tile_width; x++)
        {
            draw_tile(renderer, texture, 9, 6, room_x + x * 16 * scale, room_y);
        }
    }
    if(bitmask & SOUTH)
    {
        for(int x = 0; x < room_tile_width; x++)
        {
            draw_tile(renderer, texture, 9, 6, room_x + x * 16 * scale, room_y + (room_tile_height - 1) * 16 * scale);
        }
    }
    if(bitmask & WEST)
    {
        for(int y = 0; y < room_tile_height; y++)
        {
            draw_tile(renderer, texture, 9, 6, room_x, room_y + y * 16 * scale);
        }
    }
    if(bitmask & EAST)
    {
        for(int y = 0; y < room_tile_height; y++)
        {
            draw_tile(renderer, texture, 9, 6, room_x + (room_tile_width - 1) * 16 * scale, room_y + y * 16 * scale);
        }
    }

    return 0;
}

int get_tile(const int offsetX, const int offsetY, const int width, const int height, int *xCoord, int *yCoord)
{

    if (offsetX < 0 && offsetY < 0)
    {
        return -1;
    }

    const int tile_w = 16;
    const int tile_h = 16;

    int xvalue = tile_w * offsetX;
    int yvalue = tile_h * offsetY;

    if (xvalue > width - tile_w && yvalue > height - tile_h)
    {
        return -1;
    }

    *xCoord = xvalue;
    *yCoord = yvalue;

    return 0;
}

int draw_tile(SDL_Renderer *renderer, SDL_Texture *texture, const int tile_x, const int tile_y, const float x, const float y)
{

    SDL_FRect dest_container = {x - camera_x, y - camera_y, 16 * scale, 16 * scale};
    SDL_FRect camera = {0, 0, 512, 512};

    if(!SDL_HasIntersectionF(&dest_container, &camera))
    {
        return 0;
    }

    int width;
    int height;

    SDL_QueryTexture(texture, NULL, NULL, &width, &height);

    // First tile
    SDL_Rect offsetRect = {0, 0, 16, 16};

    get_tile(tile_x, tile_y, width, height, &offsetRect.x, &offsetRect.y);

    SDL_RenderCopyF(renderer, texture, &offsetRect, &dest_container);

    draw_tile_counter++;

    return 0;
}

int draw_actor(SDL_Renderer *renderer, SDL_Texture *texture, actor *actor)
{
    return draw_tile(renderer, texture, actor->tile_x, actor->tile_y, actor->x, actor->y);
}

int load_font(SDL_Renderer *renderer, const char *path)
{
    void *font_bytes = SDL_LoadFile(path, NULL);
    const unsigned char *char_font_bytes = (unsigned char *)font_bytes;

    SDL_Log("%d", stbtt_GetNumberOfFonts(char_font_bytes));

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, char_font_bytes, 0);

    const float font_scale = stbtt_ScaleForPixelHeight(&font_info, 36);
    SDL_Log("%f", font_scale);

    for (int i = 0; i < 95; i++)
    {

        int glyph_index = stbtt_FindGlyphIndex(&font_info, i + 32);

        stbtt_GetGlyphHMetrics(&font_info, glyph_index, &(glyphs[i].advance), NULL);
        glyphs[i].advance *= font_scale;

        const unsigned char *font_pixels = stbtt_GetGlyphBitmap(
            &font_info,
            font_scale,
            font_scale,
            glyph_index,
            &glyphs[i].rect.w,
            &glyphs[i].rect.h,
            &glyphs[i].rect.x,
            &glyphs[i].rect.y);

        if (glyphs[i].rect.w == 0 || glyphs[i].rect.h == 0)
        {
            continue;
        }

        
        const size_t memory_size = glyphs[i].rect.w * glyphs[i].rect.h * 4;
        unsigned char *memory = SDL_malloc(memory_size);

        // SDL_Log("Char %c: w %d | h: %d | x: %d | y: %d",
        //     i + 32, glyphs[i].rect.w, glyphs[i].rect.h, glyphs[i].rect.x, glyphs[i].rect.y);

        for (int j = 0; j < glyphs[i].rect.w * glyphs[i].rect.h; j++)
        {
            char v = font_pixels[j];

            memory[j * 4] = 255;
            memory[j * 4 + 1] = 255;
            memory[j * 4 + 2] = 255;
            memory[j * 4 + 3] = v;
        }
        stbtt_FreeBitmap((unsigned char *)font_pixels, NULL);

        glyphs[i].texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
            glyphs[i].rect.w, glyphs[i].rect.h);

        if (!glyphs[i].texture )
        {
            SDL_Log("Error creating Font Texture: %s", SDL_GetError());
            SDL_free(font_bytes);
            SDL_free(memory);

            return -1;
        }

        SDL_SetTextureBlendMode(glyphs[i].texture , SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(glyphs[i].texture , NULL, memory, glyphs[i].rect.w * 4);


        SDL_free(memory);
    }
    SDL_free(font_bytes);

    return 0;
}

int draw_text(SDL_Renderer *renderer, const float x, const float y, const char *str)
{

    SDL_Rect src = {0,0,0,0};
    SDL_FRect dest = {0,0,0,0};

    float curr_x = x;
    float curr_y = y;
    
    int currindec = 0;
    char c = str[0];
    
    while (c != 0)
    {
        DungeonGlyph *current_glyph = &(glyphs[c - 32]);

        dest.x = curr_x + current_glyph->rect.x;
        dest.y = curr_y + current_glyph->rect.y;
        dest.w = current_glyph->rect.w;
        dest.h = current_glyph->rect.h;

        src.w = current_glyph->rect.w;
        src.h = current_glyph->rect.h;

        SDL_SetTextureColorMod(current_glyph->texture, 255,0,0);
        
        SDL_RenderCopyF(renderer, current_glyph->texture, &src, &dest);

        curr_x += current_glyph->advance;

        currindec++;
        c = str[currindec];
    }
    
    return 0;
}