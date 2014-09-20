#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS // disable fopen/fread warnings on windows
#endif

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include "RgbBitmap.h"
#include "PvrTcEncoder.h"
#include "PvrTcDecoder.h"

/*
 Test program for the PvrTcEncoder.
*/

using namespace Javelin;

RgbBitmap *readTga(const char *filename) {
  FILE *fp = fopen(filename, "rb");

  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  unsigned char header[18];
  fread(header, 18, 1, fp);

  int w = header[12] | (header[13] << 8);
  int h = header[14] | (header[15] << 8);

  RgbBitmap *bitmap = new RgbBitmap(w, h);
  fread((void *)bitmap->data, w * h * 3, 1, fp);

  fclose(fp);

  return bitmap;
}

void writeTga(const char *filename, RgbBitmap *bitmap) {
  FILE *fp = fopen(filename, "wb");

  unsigned char header[18];
  memset(header, 0, 18);
  header[2] = 2;
  header[12] = bitmap->width & 0xff;
  header[13] = (bitmap->width >> 8) & 0xff;
  header[14] = bitmap->height & 0xff;
  header[15] = (bitmap->height >> 8) & 0xff;
  header[16] = 24;

  fwrite(header, 18, 1, fp);

  fwrite(bitmap->data, bitmap->width * bitmap->height * 3, 1, fp);

  fclose(fp);
}

void writePvr(const char *filename, RgbBitmap *bitmap) {
    FILE *fp = fopen(filename, "wb");

    unsigned char *pvrtc = new unsigned char[bitmap->GetArea()];
    PvrTcEncoder::EncodeRgb4Bpp(pvrtc, *bitmap);

    const int PVR_TYPE_PVRTC2 = 0x18;
    const int PVR_TYPE_PVRTC4 = 0x19;
    const int PVR2_MAGIC = 0x21525650;

    // PVR2 Header
    uint32_t size = 44; // sizeof PVR header
    uint32_t mipmapCount = 1;
    uint32_t flags = PVR_TYPE_PVRTC4;
    uint32_t texdatasize = bitmap->GetArea();
    uint32_t bpp = 8;
    uint32_t rmask = 255;
    uint32_t gmask = 255;
    uint32_t bmask = 255;
    uint32_t amask = 0;
    uint32_t magic = PVR2_MAGIC;
    uint32_t numtex = 1;

    fwrite(&size, 4, 1, fp);
    fwrite(&bitmap->height, 4, 1, fp);
    fwrite(&bitmap->width, 4, 1, fp);
    fwrite(&mipmapCount, 4, 1, fp);
    fwrite(&flags, 4, 1, fp);
    fwrite(&texdatasize, 4, 1, fp);
    fwrite(&bpp, 4, 1, fp);
    fwrite(&rmask, 4, 1, fp);
    fwrite(&gmask, 4, 1, fp);
    fwrite(&bmask, 4, 1, fp);
    fwrite(&amask, 4, 1, fp);
    fwrite(&magic, 4, 1, fp);
    fwrite(&numtex, 4, 1, fp);

    fwrite(pvrtc, bitmap->GetArea(), 1, fp);

    fclose(fp);
}

int main(int argc, char **argv) {
  RgbBitmap *rgbBitmap = readTga("globe.tga");

  const int area = rgbBitmap->GetArea() / 2;
  ColorRgb<unsigned char> *rgb = rgbBitmap->GetData();

  FILE *fp = fopen("globe.pvrtc", "rb");
  fseek(fp, 0, SEEK_END);
  int size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned char *globe_pvrtc = new unsigned char[size];
  fread(globe_pvrtc, size, 1, fp);
  fclose(fp);

  // Write the texture prior to compression
  writeTga("globe_before.tga", rgbBitmap);

  writePvr("globe_encoded.pvr", rgbBitmap);

  unsigned char *pvrtc = new unsigned char[area];

  memset(pvrtc, 0, area);
  PvrTcEncoder::EncodeRgb4Bpp(pvrtc, *rgbBitmap);

  std::cout << area << " : " << size << std::endl;
  for (int i = 0; i < size; ++i) {
      if (pvrtc[i] != globe_pvrtc[i]) {
          std::cout << i << " :" << (int) pvrtc[i] << " != " << (int) globe_pvrtc[i] << std::endl;
          break;
      }
  }

  PvrTcDecoder::DecodeRgb4Bpp(rgb, rgbBitmap->GetSize(), pvrtc);

  // Write the texture post compression
  writeTga("globe_after.tga", rgbBitmap);

  delete rgbBitmap;

  return 0;
}
