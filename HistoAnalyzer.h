#pragma once

#include <iostream>
#include <stdio.h>
#include "sqlite3.h"
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkGDCMImageIO.h"
#include "itkImageRegionIterator.h"
#include "gdcmScanner.h"
#include "gdcmReader.h"

class HistoAnalyzer {

public:
	typedef float VoxelType;
	typedef itk::Image<VoxelType, 3> ImageType;

private:
	std::string indir, dbpath, outdir;

	ImageType::Pointer mask;

	std::vector<std::string> slidepaths, mapnames, mappaths;
	std::vector<ImageType::IndexType> g6, g7, pin, np;

	float *vg6, *vg7, *vpin, *vnp;

public:
	HistoAnalyzer(std::string indir, std::string dbpath, std::string outdir);

	bool ReadDBMaps();
	bool ReadImageHistoDicoms();
	bool WriteVoxelArrays();
	bool WriteImageMask();

	ImageType::Pointer ReadImageMap(std::string mappath);

};