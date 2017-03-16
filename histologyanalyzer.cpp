#include <iostream>
#include <stdio.h>
#include "HistoAnalyzer.h"

using namespace std;

int main(int argc, char* argv[]) {

	if (argc < 4)
	{
		cerr << "Usage: " << argv[0] << " DicomDirectory OutputDirectory DatabasePath" << endl;
		return EXIT_FAILURE;
	}

	HistoAnalyzer ha(argv[1], argv[2], argv[3]);

	cout << "Reading parameter map db..." << endl;
	if (!ha.ReadDBMaps()) {
		cout << "\tReading db failed." << endl;
		return EXIT_FAILURE;
	}
	else {
		cout << "\tReading complete." << endl;
	}

	return EXIT_SUCCESS;

}