#pragma once

#include <iostream>
#include <stdio.h>
#include "sqlite3.h"
#include "itkImage.h"
#include "itkImageFileReader.h"

class HistoAnalyzer {

	std::string indir, dbpath, outdir;

	std::vector<std::string> slidepaths, mapnames, mappaths;
	size_t nslides, nmaps;

	float *v0, *v1;
	size_t n0, n1;

public:

	typedef double VoxelType;
	typedef itk::Image<VoxelType, 3> ImageType;

public:

	HistoAnalyzer(std::string indir, std::string dbpath, std::string outdir);

	bool GetVoxels();
	bool ReadDBMaps();
	ImageType::Pointer ReadImageMap(std::string mappath);
	bool ReadSlidesDicoms();

};