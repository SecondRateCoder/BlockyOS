#include "stdgraphics.h"

static VBEModeBlock localmode;

void tellVBEInfo(VBEModeBlock mode){localmode = mode;}

void setFontChar(Char c, char code[FontCodeSize]){
    static LINKERSECTION("FontTable") FontTable Table;
    for(uint8_t cc =0; cc < 8; ++cc){
        if(strcmp(code, Table.fonts[cc].code)){
            for(uint32_t cc_ = 0; cc_ < Table.fonts[cc].tablesize; ++cc_){
                if(Table.fonts[cc].table[cc_].c == c.c){
                    memcpy(Table.fonts[cc].table[cc_].image, c.image, sizeof(c.image[cc]) * 32);
                }
            }
        }
    }
}
Font *getFont(char code[FontCodeSize]){
    static LINKERSECTION("FontTable") FontTable Table;
    for(uint8_t cc =0; cc < 8; ++cc){
        if(strcmp(code, Table.fonts[cc].code)){
            return (Table.fonts + cc);
        }
    }
    return NULL;
}
Char *getFontChar(char code[FontCodeSize], char c){
    static LINKERSECTION("FontTable") FontTable Table;
    Font *f = getFont(code);
    for(uint32_t cc =0; cc < f->tablesize; ++cc){
        if(c == f->table[cc].c){
            return (f->table + cc);
        }
    }
    return NULL;
}
Char *getFontCharF(Font *f, char c){
    static LINKERSECTION("FontTable") FontTable Table;
    for(uint32_t cc =0; cc < f->tablesize; ++cc){
        if(c == f->table[cc].c){
            return (f->table + cc);
        }
    }
    return NULL;
}

Pixel *ChartoImage(Char *c, uint8_t w, uint8_t h){
    static Pixel out[CharImageSize];
    uint32_t cc= 0, indexer = 0;
    for(uint8_t y = 0; y < h; ++y){
        for(uint8_t x = 0; x < w; ++x){
            if(c->image[toIndex(x, y, w)]){
                out[indexer] = (Pixel){.unPacked.A = 128, .unPacked.R = 128, .unPacked.G = 128, .unPacked.B = 128};
                indexer++;
            }
            cc++;
        }
    }
}

bool putcdecoded(char c, Pixel colorOffset, uint32_t *x, uint32_t *y, Font *f){
    Char *ch;
    if((ch = getFontCharF(f, c)) != NULL){
        putimage(ChartoImage(ch, f->width, f->height), colorOffset, 0.5, f->width, f->height, *x, *y);
        return true;
    }
    return false;
}

void putimage(Pixel *image, Pixel normal, float mix, uint8_t width, uint8_t height, uint32_t x, uint32_t y){
    mix = (mix > 1? (1/mix): mix);
    for(uint32_t cc = 0; cc < (height * width) && cc < (localmode.header.VESA1_0.width * localmode.header.VESA1_0.height); ++cc){
        Pixel p = {
            .unPacked.A = mix * (normal.unPacked.A + image[cc].unPacked.A),
            .unPacked.R = mix * (normal.unPacked.R + image[cc].unPacked.R),
            .unPacked.G = mix * (normal.unPacked.G + image[cc].unPacked.G),
            .unPacked.B = mix * (normal.unPacked.B + image[cc].unPacked.B)
        };
        switch(localmode.header.VESA1_0.bits_perpixel){
            case 4: {
                ((uint8_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc, y, localmode.header.VESA1_0.width)] = packPixel(
                        p.unPacked.A, p.unPacked.R, p.unPacked.G, p.unPacked.B, 
                        localmode.header.VESA1_2.reservedMaskSize, localmode.header.VESA1_2.reservedFieldOffset, 
                        localmode.header.VESA1_2.redMaskSize, localmode.header.VESA1_2.redFieldOffset, 
                        localmode.header.VESA1_2.greenMaskSize, localmode.header.VESA1_2.greenFieldOffset, 
                        localmode.header.VESA1_2.blueMaskSize, localmode.header.VESA1_2.blueFieldOffset)
                         | (cc > (height * width)? 0: (packPixel(
                        image[cc + 1].unPacked.A, image[cc + 1].unPacked.R, image[cc + 1].unPacked.G, image[cc + 1].unPacked.B, 
                        localmode.header.VESA1_2.reservedMaskSize, localmode.header.VESA1_2.reservedFieldOffset, 
                        localmode.header.VESA1_2.redMaskSize, localmode.header.VESA1_2.redFieldOffset, 
                        localmode.header.VESA1_2.greenMaskSize, localmode.header.VESA1_2.greenFieldOffset, 
                        localmode.header.VESA1_2.blueMaskSize, localmode.header.VESA1_2.blueFieldOffset)));
                    cc++;
            }case 8: {
                ((uint8_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc, y, localmode.header.VESA1_0.width)] = packPixel(
                        p.unPacked.A, p.unPacked.R, p.unPacked.G, p.unPacked.B, 
                        localmode.header.VESA1_2.reservedMaskSize, localmode.header.VESA1_2.reservedFieldOffset, 
                        localmode.header.VESA1_2.redMaskSize, localmode.header.VESA1_2.redFieldOffset, 
                        localmode.header.VESA1_2.greenMaskSize, localmode.header.VESA1_2.greenFieldOffset, 
                        localmode.header.VESA1_2.blueMaskSize, localmode.header.VESA1_2.blueFieldOffset);
            }case 16: {
                ((uint16_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc, y, localmode.header.VESA1_0.width)] = packPixel(
                        p.unPacked.A, p.unPacked.R, p.unPacked.G, p.unPacked.B, 
                        localmode.header.VESA1_2.reservedMaskSize, localmode.header.VESA1_2.reservedFieldOffset, 
                        localmode.header.VESA1_2.redMaskSize, localmode.header.VESA1_2.redFieldOffset, 
                        localmode.header.VESA1_2.greenMaskSize, localmode.header.VESA1_2.greenFieldOffset, 
                        localmode.header.VESA1_2.blueMaskSize, localmode.header.VESA1_2.blueFieldOffset);
            }case 24: {
                ((uint8_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc, y, localmode.header.VESA1_0.width)] = p.unPacked.R;
                ((uint8_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc + 1, y, localmode.header.VESA1_0.width)] = p.unPacked.G;
                ((uint8_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc + 2, y, localmode.header.VESA1_0.width)] = p.unPacked.B;
                cc += 2;
            }case 32: {
                ((uint32_t *)localmode.header.VESA2_0.linearFrameBuffer)[
                    toIndex(x + cc, y, localmode.header.VESA1_0.width)] = packPixel(
                        p.unPacked.A, p.unPacked.R, p.unPacked.G, p.unPacked.B, 
                        localmode.header.VESA3_0.directReservedMaskSize, localmode.header.VESA3_0.directReservedMaskOffset,
                        localmode.header.VESA3_0.directRedMaskSize, localmode.header.VESA3_0.directRedMaskOffset,
                        localmode.header.VESA3_0.directGreenMaskSize, localmode.header.VESA3_0.directGreenMaskOffset,
                        localmode.header.VESA3_0.directBlueMaskSize, localmode.header.VESA3_0.directBlueMaskOffset
                );
            }
        }
    }
}