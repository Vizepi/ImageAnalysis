#ifndef __CONVOLUTION_H
#define __CONVOLUTION_H

#include <cstdint>
#include <string>

#define GrayA(i, a) ((((a) & 0xff) << 24) | (((i) & 0xff) << 16) | (((i) & 0xff) << 8) | ((i) & 0xff))
#define Gray(i) GrayA(i, 255)

#define GrayScale(r, g, b) ((r) * 0.2125 + (g) * 0.7154 + (b) * 0.0721)

class QImage;

struct Convolution
{

	struct Image
	{
	
		enum class Format
		{
			RGB, ARGB, Indexed8
		};
		
		inline 			Image	(void);
		inline 			Image	(uint32_t w, uint32_t h, Format f = Format::Indexed8);
		inline			Image	(const Image& rhs);
		virtual inline 	~Image	(void);
	
		uint32_t 	width;
		uint32_t 	height;
		Format 		format;
		uint8_t* 	pixels;
		uint32_t	colorTable[256];
	
	};
	
	struct Filter
	{
		enum class SideHandle
		{
			Zeros,
			Ones,
			Black,
			White,
			Continuous,
			Mirror,
			Repeat,
			Crop
		};
		
		uint32_t 	size;
		double* 	kernel;
		double 		divisor;
	};

	static inline uint32_t PixelSize(Convolution::Image::Format format);

	static Image* ApplyFilter(const Image& image, const Filter& filter, Filter::SideHandle sideHandle, bool multi = false);
	static Filter* Rotate(const Filter& filter);
	static Convolution::Image* Refine(const Convolution::Image& tresholded, const Convolution::Image& gradient);
	static Convolution::Image* ToGrayScale(const Convolution::Image& in);
	static Image* LoadImage(const std::string& file);
	static void SaveImage(const Image& image, const std::string& file);
	static QImage ToQImage(const Image& image);
	static Convolution::Image* Treshold(Convolution::Image* image, int tresholdMin, int tresholdMax);
	static Convolution::Image* Hough(const Convolution::Image& in, const Convolution::Image& original, Convolution::Image** accumulator, int alphaPrecision, int lineCount);

};

#include "Convolution.inl"

#endif // __CONVOLUTION_H
