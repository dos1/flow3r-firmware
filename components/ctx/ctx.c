
#include "ctx_config.h"

#ifdef CONFIG_FLOW3R_CTX_FLAVOUR_FULL

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#endif

#define CTX_IMPLEMENTATION
#include "ctx.h"

#define FB_WIDTH  240
#define FB_HEIGHT 240

static inline uint16_t
ctx_565_pack (uint8_t  red,
              uint8_t  green,
              uint8_t  blue,
              const int      byteswap);
static inline void
ctx_565_unpack (const uint16_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue,
                const int byteswap);

void st3m_ctx_merge_osd(uint16_t *restrict fb,
                            const uint8_t *osd, int ostride,
                            uint16_t *restrict osd_backup, int x0, int y0, int w, int h)
{
  uint8_t rgba[4]={0,0,0,255};
  uint32_t *rgba_32 = (uint32_t*)&rgba[0];
  for (int scanline = y0; scanline < y0 + h; scanline++)
  {
     uint16_t *fb_p = &fb[scanline * 240 + x0];
     uint32_t *osd_p = (uint32_t*)&osd[(scanline-y0) * ostride];
     uint16_t *backup_p = &osd_backup[(scanline-y0) * w];
     uint32_t *ddst = (uint32_t*)&rgba[0];
     
     for (int x = 0; x < w; x++)
     {
       *backup_p = *fb_p;
       *rgba_32 = ctx_565_unpack_32 (*fb_p, 1);
       uint32_t si_ga = ((*osd_p) & 0xff00ff00) >> 8;
       uint32_t si_rb = (*osd_p) & 0x00ff00ff;
       uint32_t si_a  = si_ga >> 16;
       uint32_t racov = si_a^255;
      *(ddst) =
     (((si_rb*255+0xff00ff+(((*ddst)&0x00ff00ff)*racov))>>8)&0x00ff00ff)|
     ((si_ga*255+0xff00ff+((((*ddst)&0xff00ff00)>>8)*racov))&0xff00ff00);
       *fb_p = ctx_565_pack(rgba[0], rgba[1], rgba[2], 1);
       fb_p++;
       osd_p++;
       backup_p++;
     }
  }
}

void st3m_ctx_unmerge_osd(uint16_t *restrict fb, const uint16_t *osd_backup, int x0, int y0, int w, int h)
{
  for (int scanline = y0; scanline < y0 + h; scanline++)
  {
     uint16_t *fb_p = &fb[scanline * 240 + x0];
     const uint16_t *backup_p = &osd_backup[(scanline-y0) * w];
     for (int x = 0; x < w; x++)
     {
       *fb_p = *backup_p;
       fb_p++;
       backup_p++;
     }
  }
}
