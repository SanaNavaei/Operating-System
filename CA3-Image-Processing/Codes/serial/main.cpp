#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

#define OUTPUT_FILE "output.bmp"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
//#pragma once

struct RGB{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

std::vector<std::vector<RGB>> pixels;
std::vector<std::vector<RGB>> horizontal_flip;
std::vector<std::vector<RGB>> checker_filter;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          pixels[i][j].red = fileReadBuffer[end - count];
          break;
        case 1:
          pixels[i][j].green = fileReadBuffer[end - count];
          break;
        case 2:
          pixels[i][j].blue = fileReadBuffer[end - count];
          break;
        }
        count += 1;
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          fileBuffer[bufferSize - count] = pixels[i][j].red;
          break;
        case 1:
          fileBuffer[bufferSize - count] = pixels[i][j].green;
          break;
        case 2:
          fileBuffer[bufferSize - count] = pixels[i][j].blue;
          break;
        }
        count += 1;
      }
  }
  write.write(fileBuffer, bufferSize);
}

void hFlipFiltering()
{
  for (int i = 0; i < rows; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      horizontal_flip[i][j].red = pixels[i][cols - 1 - j].red;
      horizontal_flip[i][j].green = pixels[i][cols - 1 - j].green;
      horizontal_flip[i][j].blue = pixels[i][cols - 1 - j].blue;
    }
  }
  pixels = horizontal_flip;
}

void checkeredFiltering()
{
  for (int i = 1; i < rows - 1; i++)
  {
    for (int j = 1; j < cols - 1; j++)
    {
      //red
      int result_red = (int)((-2) * pixels[i - 1][j - 1].red - pixels[i - 1][j].red + 0 * pixels[i - 1][j + 1].red - pixels[i][j - 1].red + pixels[i][j].red + pixels[i][j + 1].red + (0) * pixels[i + 1][j - 1].red + pixels[i + 1][j].red + 2 * pixels[i + 1][j + 1].red);
      if(result_red > 255)
        checker_filter[i][j].red = 255;
      else if(result_red < 0)
        checker_filter[i][j].red = 0;
      else
        checker_filter[i][j].red = ((-2)*pixels[i - 1][j - 1].red - pixels[i - 1][j].red + 0 * pixels[i - 1][j + 1].red
                            - pixels[i][j - 1].red + pixels[i][j].red + pixels[i][j + 1].red
                            + (0) * pixels[i + 1][j - 1].red + pixels[i + 1][j].red + 2 * pixels[i + 1][j + 1].red);

      //green
      int result_green = (int)((-2) * pixels[i - 1][j - 1].green - pixels[i - 1][j].green + 0 * pixels[i - 1][j + 1].green - pixels[i][j - 1].green + pixels[i][j + 1].green + pixels[i][j].green + (0) * pixels[i + 1][j - 1].green + pixels[i + 1][j].green + 2 * pixels[i + 1][j + 1].green);
      if(result_green > 255)
        checker_filter[i][j].green = 255;
      else if(result_green < 0)
        checker_filter[i][j].green = 0;
      else 
        checker_filter[i][j].green = ((-2)*pixels[i - 1][j - 1].green - pixels[i - 1][j].green + 0 * pixels[i - 1][j + 1].green
                            - pixels[i][j - 1].green  + pixels[i][j + 1].green  + pixels[i][j].green 
                            + (0) * pixels[i + 1][j - 1].green + pixels[i + 1][j].green  + 2 * pixels[i + 1][j + 1].green );

      //blue
      int result_blue = (int)(-2) * pixels[i - 1][j - 1].blue - pixels[i - 1][j].blue + 0 * pixels[i - 1][j + 1].blue - pixels[i][j - 1].blue + pixels[i][j].blue + pixels[i][j + 1].blue + (0) * pixels[i + 1][j - 1].blue + pixels[i + 1][j].blue + 2 * pixels[i + 1][j + 1].blue;
      if(result_blue > 255)
        checker_filter[i][j].blue = 255;
      else if(result_blue < 0)
        checker_filter[i][j].blue = 0;
      else
        checker_filter[i][j].blue = ((-2)*pixels[i - 1][j - 1].blue - pixels[i - 1][j].blue + 0 * pixels[i - 1][j + 1].blue
                            - pixels[i][j - 1].blue  + pixels[i][j].blue  + pixels[i][j + 1].blue  
                            + (0) * pixels[i + 1][j - 1].blue + pixels[i + 1][j].blue  + 2 * pixels[i + 1][j + 1].blue );
    }
  }
  pixels = checker_filter;
}

//function to generate diamond2 filtering
void diamondFiltering()
{
  for(int i = 0; i < rows;i++)
  {
    for (int j = 0; j < cols;j++)
    {
      if(i + rows/2 == (int)(rows * j / cols))
      {
        pixels[i][j].red = 255;
        pixels[i][j].green = 255;
        pixels[i][j].blue = 255;
      }
      if(i - rows/2==  (int)(rows * j / cols))
      {
        pixels[i][j].red = 255;
        pixels[i][j].green = 255;
        pixels[i][j].blue = 255;
      }
      if(i - rows/2 ==  (int)(-rows * j/ cols))
      {
        pixels[i][j].red = 255;
        pixels[i][j].green = 255;
        pixels[i][j].blue = 255;
      }
      if(i - rows/2 == (int)(-rows * j / cols + rows))
      {
        pixels[i][j].red = 255;
        pixels[i][j].green = 255;
        pixels[i][j].blue = 255;
      }
    }
  }
}

void applyFilters()
{
  auto start = std::chrono::high_resolution_clock::now();
  hFlipFiltering();
  auto end = std::chrono::high_resolution_clock::now();
  cout << "Hflip time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  start = std::chrono::high_resolution_clock::now();
  checkeredFiltering();
  end = std::chrono::high_resolution_clock::now();
  cout << "Chekered time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  start = std::chrono::high_resolution_clock::now();
  diamondFiltering();
  end = std::chrono::high_resolution_clock::now();
  cout << "Diamond time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
}

int main(int argc, char *argv[])
{
  auto start1 = std::chrono::high_resolution_clock::now();
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  pixels.assign(rows, std::vector<RGB>(cols));
  horizontal_flip.assign(rows, std::vector<RGB>(cols));
  checker_filter.assign(rows, std::vector<RGB>(cols));

  auto start = std::chrono::high_resolution_clock::now();
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
  auto end = std::chrono::high_resolution_clock::now();
  cout << "Reading time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  applyFilters();
  
  start = std::chrono::high_resolution_clock::now();
  writeOutBmp24(fileBuffer, OUTPUT_FILE, bufferSize);
  end = std::chrono::high_resolution_clock::now();
  cout << "Writing time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  auto end1 = std::chrono::high_resolution_clock::now();
  cout << "Total time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << endl;
  return 0;
}