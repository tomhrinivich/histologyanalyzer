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

	typedef double VoxelType;
	typedef itk::Image<VoxelType, 3> ImageType;

private:
	std::string indir, dbpath, outdir;

	std::vector<std::string> slidepaths, mapnames, mappaths;
	std::vector<ImageType::IndexType> g6, g7, pin, np;

	float *vg6, *vg7, *vpin, *vnp;

public:

	HistoAnalyzer(std::string indir, std::string dbpath, std::string outdir);

	bool GetVoxels();
	bool ReadDBMaps();
	bool ReadImageHistoDicoms(ImageType::Pointer img, std::string mapname, float *v0, size_t *n0);

	ImageType::Pointer ReadImageMap(std::string mappath);

};