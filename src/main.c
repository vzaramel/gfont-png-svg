#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "lodepng.h"

#define HEIGHT 60
#define WIDTH 200
unsigned char buffer[24<<20];
unsigned char screen[HEIGHT][WIDTH];

int FindBestPNGCompression(const unsigned char* image, int width, unsigned int height) {
  unsigned char* buffer = (unsigned char*)malloc(WIDTH * HEIGHT * 2);
  memset(buffer, 0, width * height * 2);

  LodePNGState state;
  lodepng_state_init(&state);
  state.encoder.filter_palette_zero = 0; //We try several filter types, including zero, allow trying them all on palette images too.
  state.encoder.add_id = false; //Don't add LodePNG version chunk to save more bytes
  state.encoder.text_compression = 1; //Not needed because we don't add text chunks, but this demonstrates another optimization setting
  state.encoder.zlibsettings.nicematch = 258; //Set this to the max possible, otherwise it can hurt compression
  state.encoder.zlibsettings.lazymatching = 1; //Definitely use lazy matching for better compression
  state.encoder.zlibsettings.windowsize = 32768; //Use maximum possible window size for best compression
  state.info_raw = lodepng_color_mode_make(LCT_GREY_ALPHA, 8);
  state.info_png.color = lodepng_color_mode_make(LCT_GREY_ALPHA, 8);

  size_t bestsize = 0;
  bool inited = false;

  int beststrategy = 0;
  LodePNGFilterStrategy strategies[4] = { LFS_ZERO, LFS_MINSUM, LFS_ENTROPY, LFS_BRUTE_FORCE };
  char* strategynames[4] = { "LFS_ZERO", "LFS_MINSUM", "LFS_ENTROPY", "LFS_BRUTE_FORCE" };

  // min match 3 allows all deflate lengths. min match 6 is similar to "Z_FILTERED" of zlib.
  int minmatches[2] = { 3, 6 };
  int bestminmatch = 0;

  int autoconverts[2] = { 0, 1 };
  char* autoconvertnames[2] = { "0", "1" };
  int bestautoconvert = 0;

  int bestblocktype = 0;

  // Try out all combinations of everything
  for(int i = 0; i < 4; i++)   //filter strategy
  for(int j = 0; j < 2; j++)   //min match
  for(int k = 0; k < 2; k++)   //block type (for small images only)
  for(int l = 0; l < 2; l++) { //color convert strategy
    if(bestsize > 3000 && (k > 0 || l > 0)) continue; /* these only make sense on small images */
    state.encoder.filter_strategy = strategies[i];
    state.encoder.zlibsettings.minmatch = minmatches[j];
    state.encoder.zlibsettings.btype = k == 0 ? 2 : 1;
    state.encoder.auto_convert = autoconverts[l];
    size_t out_size;
    unsigned error = lodepng_encode(&buffer, &out_size, image, WIDTH, HEIGHT, &state);

    if(error) {
      printf("encoding error %d: %s\n", error, lodepng_error_text(error));
      return -1;
    }

    if(!inited || out_size < bestsize)
    {
      bestsize = out_size;
      beststrategy = i;
      bestminmatch = state.encoder.zlibsettings.minmatch;
      bestautoconvert = l;
      bestblocktype = state.encoder.zlibsettings.btype;
      // temp.swap(buffer);
      inited = true;
    }
  }

  printf("Chosen filter strategy: %s\n",strategynames[beststrategy]) ;
  printf("Chosen min match: %d\n", bestminmatch);
  printf("Chosen block type: %d\n", bestblocktype);
  printf("Chosen auto convert: %s\n", autoconvertnames[bestautoconvert]);
  printf("Chosen size: %s\n", autoconvertnames[bestautoconvert]);

  return 0;
}

int CompressPNG(unsigned char **out, size_t *out_size, const unsigned char* image, int width, unsigned int height) {
  
  LodePNGState state;
  lodepng_state_init(&state);
  state.encoder.filter_palette_zero = 0; //We try several filter types, including zero, allow trying them all on palette images too.
  state.encoder.add_id = false; //Don't add LodePNG version chunk to save more bytes
  state.encoder.text_compression = 1; //Not needed because we don't add text chunks, but this demonstrates another optimization setting
  state.encoder.zlibsettings.nicematch = 258; //Set this to the max possible, otherwise it can hurt compression
  state.encoder.zlibsettings.lazymatching = 1; //Definitely use lazy matching for better compression
  state.encoder.zlibsettings.windowsize = 32768; //Use maximum possible window size for best compression
  state.encoder.filter_strategy = LFS_ZERO;
  state.encoder.zlibsettings.minmatch = 3;
  state.encoder.zlibsettings.btype = 2;
  state.encoder.auto_convert = 0;
  state.info_raw = lodepng_color_mode_make(LCT_GREY_ALPHA, 8);
  state.info_png.color = lodepng_color_mode_make(LCT_GREY_ALPHA, 8);

  unsigned error = lodepng_encode(out, out_size, image, WIDTH, HEIGHT, &state);

  if(error) {
    printf("encoding error %d: %s\n", error, lodepng_error_text(error));
    return -1;
  }

  return 0;
}

typedef struct {
  stbtt_uint32 Tag;
  stbtt_uint32 MinValue;
  stbtt_uint32 DefaultValue;
  stbtt_uint32 MaxValue;
  stbtt_uint16 Flags;
  stbtt_uint16 NameId;
} font_variations_axis;

typedef struct {
  stbtt_uint16 NameID;
  stbtt_uint16 Flags;
  stbtt_uint32 *Coord;
  stbtt_uint16 PsNameID;
} font_variations_instance;

typedef struct {
  stbtt_uint16 MajorVersion;
  stbtt_uint16 MinorVersion;
  stbtt_uint16 OffsetToData;
  stbtt_uint16 CountSizePairs;
  stbtt_uint16 AxisCount;
  stbtt_uint16 AxisSize;
  stbtt_uint16 InstanceCount;
  stbtt_uint16 InstanceSize;
} font_variations_header;

typedef struct {
  font_variations_header Header;
  font_variations_axis *Axis;
  font_variations_instance *Instances;
} font_variation_table;

typedef struct {
  char* buffer;
  unsigned int len;
  unsigned int cap;
  // char* cursor;
} string_builder;

void AppendStr(string_builder *S, const char *str) {
  if(!S->buffer) {
    S->buffer = (char *)malloc(256);
    S->len = 0;
    S->cap = 100;
  }
  if(S->len + strlen(str) >= S->cap) {
    S->cap =  S->len + strlen(str) * 2;
    S->buffer = (char *)realloc(S->buffer, S->cap);
  } 
  char* Cursor = S->buffer + S->len;
  S->len += sprintf_s(Cursor, S->cap - S->len, "%s", str);
}

void AppendNumber(string_builder *S, int number) {
  char buff[10];
  sprintf_s(buff, 10, "%d", number);
  AppendStr(S, buff);
}


void SVG_Header(string_builder *SVG) {
  AppendStr(SVG, "<svg xmlns='http://www.w3.org/2000/svg' version='1.1'>");
  AppendStr(SVG, "<g fill-rule='nonzero' transform='translate(0, 80) scale(0.05)'>");
  AppendStr(SVG, "<path d='");
}

void SVG_Footer(string_builder *SVG) {
  AppendStr(SVG, "'/>");
  AppendStr(SVG, "</g></svg>");
}
void SVG_Outline(string_builder *SVG, stbtt_vertex *vertices, int num_verts, int x_pos) {
  
  int i,num_contours=0,start=0;
  int num_points=0;

  for (i=0; i < num_verts; ++i) {
    if (vertices[i].type == STBTT_vmove){
      ++num_contours;
      ++num_points;
    }
    else if (vertices[i].type == STBTT_vline){
      ++num_points;
    }
  }
    

  char buff[100];
  float x=0,y=0;
  num_points = 0;
  char mode = 'a';
  // n= -1;
  for (int i = 0; i < num_verts; ++i) {
      switch (vertices[i].type) {
        case STBTT_vmove:
            start = num_points++;
            x = vertices[i].x, y = vertices[i].y;
            snprintf(buff, 100, " M%.0f,%.0f", x + x_pos, -y);

            AppendStr(SVG, buff);
            break;
        case STBTT_vline:
            x = vertices[i].x, y = vertices[i].y;
            snprintf(buff, 100, " L%.0f,%.0f", x + x_pos, -y);
            AppendStr(SVG, buff);
            break;
        case STBTT_vcurve:
            x = vertices[i].x, y = vertices[i].y;
            snprintf(buff, 100, " Q%d,%d %.0f,%.0f", vertices[i].cx + x_pos, -vertices[i].cy, x + x_pos, -y);
            AppendStr(SVG, buff);

            break;
        case STBTT_vcubic:
            printf("STBTT_vcubic NOT IMPLEMENTED!");
            break;
      }
  }

}

int main(int arg, char **argv)
{
   stbtt_fontinfo font;
   int i,j,ascent,descent, baseline,ch=0;
   float scale, xpos=2; // leave a little padding in case the character extends left
   char *text = "Bebas Neue"; // intentionally misspelled to show 'lj' brokenness

  //  fread(buffer, 1, 1000000, fopen("./lib/fonts/ofl/architectsdaughter/ArchitectsDaughter-Regular.ttf", "rb"));
   fread(buffer, 1, 1000000, fopen("./lib/fonts/ofl/bebasneue/BebasNeue-Regular.ttf", "rb"));
  //  fread(buffer, 1, 1000000, fopen("./lib/fonts/ofl/aladin/Aladin-Regular.ttf", "rb"));
  //  fread(buffer, 1, 1000000, fopen("./lib/fonts/apache/roboto/Roboto[wdth,wght].ttf", "rb"));
  //  fread(buffer, 1, 1000000, fopen("./lib/fonts/apache/roboto/static/RobotoCondensed-Regular.ttf", "rb"));
   stbtt_InitFont(&font, buffer, 0);
   scale = stbtt_ScaleForPixelHeight(&font, 60);
   stbtt_GetFontVMetrics(&font, &ascent, &descent,0);

  font_variation_table FontVariationTable;
  stbtt_uint8 *p = (font.data + font.fvar);
  font_variations_header *Header = &FontVariationTable.Header;
  Header->AxisCount = ttUSHORT(p + 8);
  Header->InstanceCount = ttUSHORT(p + 12);
  Header->InstanceSize = ttUSHORT(p + 14);
  for(int AxisIndex = 0; AxisIndex < Header->AxisCount; AxisIndex++) {
    stbtt_uint32 Tag = ttULONG(p + 16 + AxisIndex * 20);
    printf("Tag %x\n", Tag);
  }

  baseline = (int) (ascent*scale);
  
  string_builder SVG = {0};
  SVG_Header(&SVG);

  while (text[ch]) {
      int advance,lsb,x0,y0,x1,y1;
      float x_shift = xpos - (float) floor(xpos);
      int GlyphIndex =  stbtt_FindGlyphIndex(&font, text[ch]);
      
      stbtt_GetGlyphHMetrics(&font, GlyphIndex, &advance, &lsb);
      stbtt_GetGlyphBitmapBoxSubpixel(&font, GlyphIndex, scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
      stbtt_MakeGlyphBitmapSubpixel(&font, &screen[baseline + y0][(int) xpos + x0], x1-x0,y1-y0, WIDTH, scale,scale,x_shift,0, GlyphIndex);
      
      stbtt_vertex *vertices;
      int num_verts = stbtt_GetGlyphShape(&font, GlyphIndex, &vertices);

      SVG_Outline(&SVG, vertices, num_verts, xpos / scale);

      STBTT_free(vertices, font.userdata);

      xpos += (advance * scale);
      if (text[ch+1])
         xpos += scale*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
      ++ch;
  }

  unsigned int channels = 2;
  unsigned char* image = (unsigned char*)malloc(WIDTH * HEIGHT * channels);
  memset(image, 0, WIDTH * HEIGHT * channels);
  
  /*if there's an error, display it*/
  for (j=0; j < HEIGHT; ++j) {
    for (i=0; i < WIDTH; ++i) {
      if(screen[j][i] > 0) {
        image[j * WIDTH * channels + i * channels + 0] = 0;
        image[j * WIDTH * channels + i * channels + 1] = screen[j][i];
      }
    }
  }

  SVG_Footer(&SVG);
  printf("SVG => %.*s\n\n", SVG.len, SVG.buffer);
  lodepng_save_file((unsigned char*)SVG.buffer, SVG.len, "./out/svg/image.svg");

  // unsigned error = FindBestPNGCompression(image, WIDTH, HEIGHT);
  // if(error) printf("error %u\n", error);

  unsigned char* buffer;

  size_t out_size;
  unsigned error = CompressPNG(&buffer, &out_size, image, WIDTH, HEIGHT);
  if(error) printf("error %u\n", error);

  if(!error) error = lodepng_save_file(buffer, out_size, "./out/image.png");



  return 0;
}