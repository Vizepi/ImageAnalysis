#include "Convolution.h"

inline Convolution::Image::Image(void)
	: width(0)
	, height(0)
	, format(Convolution::Image::Format::Indexed8)
	, pixels(nullptr)
{
	
}

inline Convolution::Image::Image(uint32_t w, uint32_t h, Convolution::Image::Format f)
	: width(w)
	, height(h)
	, format(f)
	, pixels(nullptr)
{
	uint32_t pixelSize = Convolution::PixelSize(format);
	if(format == Convolution::Image::Format::Indexed8)
	{
		for(uint32_t i = 0; i < 256; ++i)
		{
			colorTable[i] = Gray(i);
		}
	}
	pixels = new uint8_t[width * height * pixelSize];
}

inline Convolution::Image::Image(const Convolution::Image& rhs)
	: width(rhs.width)
	, height(rhs.height)
	, format(rhs.format)
	, pixels(nullptr)
{
	for(uint32_t i = 0; i < 256; ++i)
	{
		colorTable[i] = rhs.colorTable[i];
	}
	if(width * height != 0)
	{
		uint32_t pixelSize = Convolution::PixelSize(format);
		uint32_t size = width * height * pixelSize;
		pixels = new uint8_t[size];
		for(uint32_t i = 0; i < size; ++i)
		{
			pixels[i] = rhs.pixels[i];
		}
	}
}

/*virtual*/ inline Convolution::Image::~Image(void)
{
	if(pixels != nullptr)
	{
		delete [] pixels;
		pixels = nullptr;
	}
}

/*static*/ inline uint32_t Convolution::PixelSize(Convolution::Image::Format format)
{
	switch(format)
	{
	case Convolution::Image::Format::RGB:		return 3;
	case Convolution::Image::Format::ARGB:		return 4;
	case Convolution::Image::Format::Indexed8:	return 1;
	}
	return 0;
}
