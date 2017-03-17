#include "HistoAnalyzer.h"

HistoAnalyzer::HistoAnalyzer(std::string indir, std::string dbpath, std::string outdir):
	indir(indir),
	dbpath(dbpath),
	outdir(outdir){
}


bool HistoAnalyzer::ReadDBMaps() {

	//open the sqlite db
	sqlite3* db;
	if (sqlite3_open(dbpath.c_str(), &db) != SQLITE_OK) {
		return  false;
	}

	//prepare stmt
	char buffer0[] = "SELECT "
		"ParameterMapName, "
		"ImageFileName "
		"FROM parametermaps "
		"WHERE "
		"ParameterMapName = 'cgrad' OR "
		"ParameterMapName = 'ktrans_lrrm' OR "
		"ParameterMapName = 'kep_lrrm' OR "
		"ParameterMapName = 've_lrrm' OR "
		"ParameterMapName = 'ktrans_nlrrm' OR "
		"ParameterMapName = 'kep_nlrrm' OR "
		"ParameterMapName = 've_nlrrm' OR "
		"ParameterMapName = 'r_nlrrm' OR "
		"ParameterMapName = 'ktrans_laif' OR "
		"ParameterMapName = 'kep_laif' OR "
		"ParameterMapName = 've_laif' OR "
		"ParameterMapName = 'ktrans_nlaif' OR "
		"ParameterMapName = 'kep_nlaif' OR "
		"ParameterMapName = 've_nlaif' OR "
		"ParameterMapName = 'r_nlaif' OR "
		"ParameterMapName = 'cgrad_c' OR "
		"ParameterMapName = 'ktrans_lrrm_c' OR "
		"ParameterMapName = 'kep_lrrm_c' OR "
		"ParameterMapName = 've_lrrm_c' OR "
		"ParameterMapName = 'ktrans_nlrrm_c' OR "
		"ParameterMapName = 'kep_nlrrm_c' OR "
		"ParameterMapName = 've_nlrrm_c' OR "
		"ParameterMapName = 'r_nlrrm_c' OR "
		"ParameterMapName = 'ktrans_laif_c' OR "
		"ParameterMapName = 'kep_laif_c' OR "
		"ParameterMapName = 've_laif_c' OR "
		"ParameterMapName = 'ktrans_nlaif_c' OR "
		"ParameterMapName = 'kep_nlaif_c' OR "
		"ParameterMapName = 've_nlaif_c' OR "
		"ParameterMapName = 'r_nlaif_c';";


	sqlite3_stmt *stmt;
	if (sqlite3_prepare(db, buffer0, strlen(buffer0), &stmt, 0) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return  false;
	}

	//read rows
	while (true) {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			std::string t0((const char *)sqlite3_column_text(stmt, 0));
			std::string t1((const char *)sqlite3_column_text(stmt, 1));
			mapnames.push_back(t0);
			mappaths.push_back(t1);
		}
		else {
			break;
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return true;

}

bool HistoAnalyzer::ReadImageHistoDicoms() {

	//read first image for dimensions
	HistoAnalyzer::ImageType::Pointer img;
	try {
		 img = HistoAnalyzer::ReadImageMap(mappaths[0]);
	}
	catch (itk::ExceptionObject & e) {
		return false;
	}

	//allocate mask
	mask = HistoAnalyzer::ImageType::New();
	mask->SetRegions(img->GetLargestPossibleRegion());
	mask->Allocate();
	mask->SetOrigin(img->GetOrigin());
	mask->SetDirection(img->GetDirection());
	mask->SetSpacing(img->GetSpacing());
	mask->FillBuffer(0.0);
	mask->Update();

	//histology dicom list
	gdcm::Directory d;
	if (d.Load(indir) < 1) return false;

	//scan dicoms
	gdcm::Scanner s;
	s.AddTag(gdcm::Tag(0x20, 0xd));
	if (!s.Scan(d.GetFilenames())) return false;

	gdcm::Directory::FilenamesType filenames = s.GetKeys();
	if (filenames.size() < 1) return false;

	typedef itk::Vector<uint8_t, 3> PixelType;
	typedef itk::Image<PixelType, 3> RGBImageType;
	typedef itk::ImageFileReader<RGBImageType> ReaderType;
	typedef itk::GDCMImageIO ImageIOType;

	//loop through slides
	for (int i = 0; i < filenames.size(); i++) {

		ReaderType::Pointer reader = ReaderType::New();
		ImageIOType::Pointer gdcmImageIO = ImageIOType::New();
		RGBImageType::Pointer img_s = RGBImageType::New();
		
		reader->SetFileName(filenames[i]);
		reader->SetImageIO(gdcmImageIO);
		
		try {
			reader->Update();
		}
		catch (itk::ExceptionObject & e) {
			return false;
		}
		
		img_s = reader->GetOutput();

		//loop through index values
		RGBImageType::RegionType rgbregion = img_s->GetLargestPossibleRegion();
		itk::ImageRegionIterator<RGBImageType> rgbimageiterator(img_s, rgbregion);

		PixelType v;
		RGBImageType::IndexType ind0;
		HistoAnalyzer::ImageType::IndexType ind1;
		RGBImageType::PointType p;
		while (!rgbimageiterator.IsAtEnd())
		{

			v = rgbimageiterator.Get();
			ind0 = rgbimageiterator.GetIndex();

			if(ind0[1] <= 1600) {
				if (v[0] == 0 && v[1] == 0 && v[2] == 0) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 0);
				}
				else if (v[0] == 51 && v[1] == 51 && v[2] == 51) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 1);
				}
				else if (v[0] == 0 && v[1] == 128 && v[2] == 0) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 2);
				}
				else if (
					(v[0] == 0 && v[1] == 255 && v[2] == 0) ||
					(v[0] == 128 && v[1] == 255 && v[2] == 128) ||
					(v[0] == 115 && v[1] == 115 && v[2] == 0) ||
					(v[0] == 166 && v[1] == 166 && v[2] == 0) ||
					(v[0] == 255 && v[1] == 255 && v[2] == 0) ||
					(v[0] == 255 && v[1] == 255 && v[2] == 102) ||
					(v[0] == 0 && v[1] == 0 && v[2] == 179) ||
					(v[0] == 26 && v[1] == 26 && v[2] == 255) ||
					(v[0] == 102 && v[1] == 102 && v[2] == 255)
					) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 3);
				}
				else if (v[0] == 128 && v[1] == 128 && v[2] == 128) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 4);
				}
				else {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 1);
				}
			}

			++rgbimageiterator;

		}

	}

	//loop through volume 
	HistoAnalyzer::ImageType::RegionType maskregion = mask->GetLargestPossibleRegion();
	itk::ImageRegionIterator<HistoAnalyzer::ImageType> maskiterator(mask, maskregion);
	int v;

	while (!maskiterator.IsAtEnd())
	{

		v = (int)maskiterator.Get();

		switch (v) {
		case 1:
			np.push_back(maskiterator.GetIndex());
			break;
		case 2:
			g6.push_back(maskiterator.GetIndex());
			break;
		case 3:
			g7.push_back(maskiterator.GetIndex());
			break;
		case 4:
			pin.push_back(maskiterator.GetIndex());
			break;
		}

		++maskiterator;

	}

	return true; 

}

bool HistoAnalyzer::WriteVoxelArrays() {

	//allocate memory to hold voxel values
	if (g6.size() > 10) vg6 = (float*)calloc(g6.size(), sizeof(float));
	if (g7.size() > 10) vg7 = (float*)calloc(g7.size(), sizeof(float));
	if (np.size() > 10) vnp = (float*)calloc(np.size(), sizeof(float));
	if (pin.size() > 10) vpin = (float*)calloc(pin.size(), sizeof(float));

	float mg6, mg7, mpin, mnp, sg6, sg7, spin, snp;

	for (int i = 0; i < mapnames.size(); i++) {

		HistoAnalyzer::ImageType::Pointer img = HistoAnalyzer::ReadImageMap(mappaths[i]);

		if (g6.size() > 10) {
			for (int i = 0; i < g6.size(); i++) {
				vg6[i] = img->GetPixel(g6[i]);
			}
			mg6 = HistoAnalyzer::Mean<float>(vg6, g6.size());
			sg6 = HistoAnalyzer::STD<float>(vg6, g6.size(), mg6);
		}

		if (g7.size() > 10) {
			for (int i = 0; i < g7.size(); i++) {
				vg7[i] = img->GetPixel(g7[i]);
			}
			mg7 = HistoAnalyzer::Mean<float>(vg7, g7.size());
			sg7 = HistoAnalyzer::STD<float>(vg7, g7.size(), mg7);
		}

		if (np.size() > 10) {
			for (int i = 0; i < np.size(); i++) {
				vnp[i] = img->GetPixel(np[i]);
			}
			mnp = HistoAnalyzer::Mean<float>(vnp, np.size());
			snp = HistoAnalyzer::STD<float>(vnp, np.size(), mnp);
		}

		if (pin.size() > 10) {
			for (int i = 0; i < pin.size(); i++) {
				vpin[i] = img->GetPixel(pin[i]);
			}
			mpin = HistoAnalyzer::Mean<float>(vpin, pin.size());
			spin = HistoAnalyzer::STD<float>(vpin, pin.size(), mpin);
		}

		//write voxel values to disk

		//write mean values to DB

	}

	return true;

}

bool HistoAnalyzer::WriteImageMask() {

	//write mask for debugging
	typedef itk::ImageFileWriter<HistoAnalyzer::ImageType> WriterType;
	WriterType::Pointer writer = WriterType::New();

	writer->SetInput(mask);
	writer->SetFileName(outdir + "\\histo_mask.nrrd");

	try {
		writer->Update();
	}
	catch (itk::ExceptionObject & e) {
		return false;
	}

	return true;
}


HistoAnalyzer::ImageType::Pointer HistoAnalyzer::ReadImageMap(std::string mappath) {

	typedef itk::ImageFileReader<HistoAnalyzer::ImageType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();

	reader->SetFileName(mappath);
	reader->Update();

	return reader->GetOutput();

}

template<class T> T HistoAnalyzer::Mean(T* voxels, size_t nvoxels) {
	float cumsum = 0;
	for (int j = 0; j < nvoxels; j++) {
		cumsum += (float)voxels[j];
	}
	float mean = cumsum / (float)nvoxels;
	return (T)mean;
}

template<class T> T HistoAnalyzer::STD(T* voxels, size_t nvoxels, T mean) {
	T cumsum = 0;
	for (int j = 0; j < nvoxels; j++) {
		cumsum += pow(voxels[j] - mean, 2.0);
	}
	return sqrt(cumsum / (nvoxels - 1));
}