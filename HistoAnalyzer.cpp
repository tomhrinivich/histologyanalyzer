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
				if (v[0] == 51 && v[1] == 51 && v[2] == 51) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 1);
				}
				else if (v[0] == 0 && v[1] == 128 && v[2] == 0) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 2);
				}
				else if (v[0] == 128 && v[1] == 128 && v[2] == 128) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 4);
				}
				else if (v[0] == 0 && v[1] == 0 && v[2] == 0) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 0);
				}
				else if (v[0] > 0 || v[1] > 0 || v[2] > 0) {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 1);
				}
				else {
					img_s->TransformIndexToPhysicalPoint(ind0, p);
					mask->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind1, 0);
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
		case 2:
			g6.push_back(maskiterator.GetIndex());
		case 3:
			g7.push_back(maskiterator.GetIndex());
		case 4:
			pin.push_back(maskiterator.GetIndex());
		}

		++maskiterator;

	}

	return true; 

}

bool HistoAnalyzer::WriteVoxelArrays() {

	//allocate memory to hold voxel values


	for (int i = 0; i < mapnames.size(); i++) {

		HistoAnalyzer::ImageType::Pointer img = HistoAnalyzer::ReadImageMap(mappaths[i]);

		//calculate mean values + SD

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