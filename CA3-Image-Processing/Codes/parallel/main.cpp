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
// #pragma once

struct RGB
{
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
int end;
char *fileBuffer;
int bufferSize;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols)
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

void* getPixlesFromBMP24(void *threadNumber)
{
  intptr_t threadNum = (intptr_t)threadNumber;
  int startRow = threadNum * rows / 8;
  int endRow = (threadNum + 1) * rows / 8;

  int extra = cols % 4;
  int count = (threadNum * (rows / 8) * (cols + extra) * 3) + 1;
  int end = bufferSize;
  for (int i = startRow; i < endRow; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          pixels[i][j].red = fileBuffer[end - count];
          break;
        case 1:
          pixels[i][j].green = fileBuffer[end - count];
          break;
        case 2:
          pixels[i][j].blue = fileBuffer[end - count];
          break;
        }
        count += 1;
      }
  }
  pthread_exit(0);
}

void* writeOutBmp24(void* threadNumber)
{
  std::ofstream write(OUTPUT_FILE);
  if (!write)
  {
    cout << "Failed to write " << OUTPUT_FILE << endl;
    pthread_exit(0);
  }

  intptr_t threadNum = (intptr_t)threadNumber;
  int startRow = threadNum * rows / 8;
  int endRow = (threadNum + 1) * rows / 8;

  int extra = cols % 4;
  int count = (threadNum * (rows/8) * (cols + extra) * 3) + 1;
  for (int i = startRow; i < endRow; i++)
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
  pthread_exit(0);
}

void *hFlipFiltering(void *threadNumber)
{
  intptr_t threadNum = (intptr_t)threadNumber;
  int startRow = threadNum * rows / 8;
  int endRow = (threadNum + 1) * rows / 8;
  
  for (int i = startRow; i < endRow; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      horizontal_flip[i][j].red = pixels[i][cols - 1 - j].red;
      horizontal_flip[i][j].green = pixels[i][cols - 1 - j].green;
      horizontal_flip[i][j].blue = pixels[i][cols - 1 - j].blue;
    }
  }
  for (int i = startRow; i < endRow; i++)
  {
    for (int j = 0; j < cols; j++)
    {
      pixels[i][j].red = horizontal_flip[i][j].red;
      pixels[i][j].green = horizontal_flip[i][j].green;
      pixels[i][j].blue = horizontal_flip[i][j].blue;
    }
  }
  pthread_exit(0);
}

void* checkeredFiltering(void *threadNumber)
{
  intptr_t threadNum = (intptr_t)threadNumber;
  int startRow = threadNum % 2 == 0 ? 0 : rows / 2;
  int endRow = threadNum % 2 == 0 ? rows / 2 : rows;
  int startCol = threadNum < 2 ? 0 : cols / 2;
  int endCol = threadNum < 2 ? cols / 2 : cols;

  for (int i = startRow + 1; i < endRow - 1; i++)
  {
    for (int j = startCol + 1; j < endCol - 1; j++)
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
  for(int i = startRow + 1; i < endRow - 1; i++)
  {
    for(int j = startCol + 1; j < endCol - 1; j++)
    {
      pixels[i][j].red = checker_filter[i][j].red;
      pixels[i][j].green = checker_filter[i][j].green;
      pixels[i][j].blue = checker_filter[i][j].blue;
    }
  }
  pthread_exit(0);
}

void *diamondFiltering(void *threadNumber)
{
  intptr_t threadNum = (intptr_t)threadNumber;
  int startCol = threadNum < 2 ? 0 : cols / 2;
  int endCol = threadNum < 2 ? cols / 2 : cols;
  int startRow = threadNum % 2 == 0 ? 0 : rows / 2;
  int endRow = threadNum % 2 == 0 ? rows / 2 : rows;
  for (int i = startRow; i < endRow; i++)
  {
    for (int j = startCol; j < endCol; j++)
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
  pthread_exit(0);
}

void applyFilters()
{
  pthread_t hFlipThread[8];
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 8; i++)
  {
    int threadNumber = i;
    pthread_create(&hFlipThread[i], NULL, hFlipFiltering,(void *)(intptr_t)threadNumber);
  }
  for (int i = 0; i < 8; i++)
    pthread_join(hFlipThread[i], NULL);
  auto end = std::chrono::high_resolution_clock::now();
  cout << "Hflip time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  pthread_t checkeredThread[4];
  checker_filter = pixels;
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 4; i++)
  {
    int threadNumber = i;
    pthread_create(&checkeredThread[i], NULL, checkeredFiltering,(void *)(intptr_t)threadNumber);
  }
  for (int i = 0; i < 4; i++)
    pthread_join(checkeredThread[i], NULL);
  end = std::chrono::high_resolution_clock::now();
  cout << "Checkered time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  pthread_t diamondThread[4];
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 4; i++)
  {
    int threadNumber = i;
    pthread_create(&diamondThread[i], NULL, diamondFiltering,(void *)(intptr_t)threadNumber);
  }
  for (int i = 0; i < 4; i++)
    pthread_join(diamondThread[i], NULL);
  end = std::chrono::high_resolution_clock::now();
  cout << "Diamond time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;
}

int main(int argc, char *argv[])
{
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols))
  {
    cout << "File read error" << endl;
    return 1;
  }
  pixels.assign(rows, std::vector<RGB>(cols));
  horizontal_flip.assign(rows, std::vector<RGB>(cols));
  checker_filter.assign(rows, std::vector<RGB>(cols));

  auto start1 = std::chrono::high_resolution_clock::now();
  //read images
  auto start = std::chrono::high_resolution_clock::now();
  pthread_t readingThread[8];
  for (int i = 0; i < 8; i++)
  {
    int threadNumber= i;
    pthread_create(&readingThread[i], NULL, getPixlesFromBMP24, (void *)(intptr_t)threadNumber);
  }
  for (int i = 0; i < 8; i++)
    pthread_join(readingThread[i], NULL);
  auto end = std::chrono::high_resolution_clock::now();
  cout << "Reading time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  //apply filters
  applyFilters();

  //write to image
  start = std::chrono::high_resolution_clock::now();
  pthread_t writingThread[8];
  for (int i = 0; i < 8; i++)
  {
    int threadNumber= i;
    pthread_create(&writingThread[i], NULL, writeOutBmp24, (void *)(intptr_t)threadNumber);
  }
  for (int i = 0; i < 8; i++)
    pthread_join(writingThread[i], NULL);
  end = std::chrono::high_resolution_clock::now();
  cout << "Writing time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << endl;

  auto end1= std::chrono::high_resolution_clock::now();
  cout << "Total time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << endl;

  return 0;
}