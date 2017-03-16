#pragma once

#include <iostream>
#include <stdio.h>
#include "sqlite3.h"
#include "itkImage.h"

class HistoAnalyzer {

	std::string indir, dbpath, outdir;

	std::vector<std::string> slidepaths, mapnames, mappaths;
	size_t nslides, nmaps;

	float *v0, *v1;
	size_t n0, n1;

public:

	HistoAnalyzer(std::string indir, std::string dbpath, std::string outdir);

	bool ReadDBMaps();

	bool ReadSlidesDicoms();

};