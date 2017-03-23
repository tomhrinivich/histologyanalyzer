#pragma once

#include <iostream>
#include <stdio.h>
#include "sqlite3.h"
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkGDCMImageIO.h"
#include "itkImageRegionIterator.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryBallStructuringElement.h" 
#include "itkAffineTransform.h"
#include "itkTransformFileReader.h"
#include "itkResampleImageFilter.h"
#include "gdcmScanner.h"
#include "gdcmReader.h"

class HistoAnalyzer {

public:
	typedef float VoxelType;
	typedef itk::Image<VoxelType, 3> ImageType;

private:
	std::string indir, dbpath, maskpath, outdir;

	ImageType::Pointer mask;

	std::vector<std::string> slidepaths, mapnames, mappaths;
	std::vector<ImageType::IndexType> g6, g7, pin, np;

	float *vg6, *vg7, *vpin, *vnp, *mg6, *mg7, *mpin, *mnp, *sg6, *sg7, *spin, *snp;

public:
	HistoAnalyzer(std::string indir, std::string dbpath, std::string maskpath, std::string outdir);

	bool ReadDBMaps();
	bool ReadImageHistoDicoms();
	bool WriteVoxelArrays();
	bool WriteImageMask();
	bool WriteDBMeanSD();

	ImageType::Pointer ReadImageMap(std::string mappath);
	ImageType::Pointer ReadImage3TP(std::string mappath);
	template<class T> T Mean(T* voxels, size_t nvoxels);
	template<class T> T STD(T* voxels, size_t nvoxels, T mean);
	bool WriteVoxelsScalar(float * C, size_t n_v, std::string binaryfilename, std::string textfilename);

};