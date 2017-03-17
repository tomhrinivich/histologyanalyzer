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

bool HistoAnalyzer::GetVoxels() {

	typedef itk::Image<float, 3>  MaskImageType;
	typedef itk::ImageFileWriter<MaskImageType> WriterType;

	HistoAnalyzer::ImageType::Pointer img = HistoAnalyzer::ImageType::New();

	//create pointers for voxel values
	float v0[3];
	size_t n0;

	//read first image for dimensions
	img = HistoAnalyzer::ReadImageMap(mappaths[0]);
	
	//allocate mask
	MaskImageType::Pointer mask = MaskImageType::New();
	mask->SetRegions(img->GetLargestPossibleRegion());
	mask->Allocate();
	mask->SetOrigin(img->GetOrigin());
	mask->SetDirection(img->GetDirection());
	mask->SetSpacing(img->GetSpacing());

	//determine indices of interest from histology
	if (!HistoAnalyzer::ReadImageHistoDicoms(mask)) return false;

	for (int i = 0; i < mapnames.size(); i++) {

		img = HistoAnalyzer::ReadImageMap(mappaths[i]);

		

		//calculate mean values + SD

		//write voxel values to disk

		//write mean values to DB

	}

	return true;

}

bool HistoAnalyzer::ReadImageHistoDicoms(HistoAnalyzer::ImageType::Pointer mask) {

	//file list
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

	typedef itk::Image<float, 3>  MaskImageType;
	typedef itk::ImageFileWriter<MaskImageType> WriterType;

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

		//get image size
		RGBImageType::RegionType rgbregion = img_s->GetLargestPossibleRegion();
		itk::ImageRegionIterator<RGBImageType> rgbimageiterator(img_s, rgbregion);



		PixelType v;
		RGBImageType::IndexType ind;
		HistoAnalyzer::ImageType::IndexType ind1;
		RGBImageType::PointType p;
		std::stringstream ss;
		std::string outFileName;
		while (!rgbimageiterator.IsAtEnd())
		{

			v = rgbimageiterator.Get();
			ind = rgbimageiterator.GetIndex();

			if(ind[1] <= 1600) {
				if (v[1] == 128) {
					img_s->TransformIndexToPhysicalPoint(ind, p);
					img->TransformPhysicalPointToIndex(p, ind1);
					mask->SetPixel(ind, img->GetPixel(ind1));
				}
				else {
					mask->SetPixel(ind, 0);
				}
			}
			else {
				mask->SetPixel(ind, 0);
			}

			++rgbimageiterator;

		}

		//export filename
		ss.str(std::string());
		ss << std::setw(2) << std::setfill('0') << i + 1;
		outFileName = outdir + "\\" + mapname + ss.str() + ".nrrd";

		WriterType::Pointer writer = WriterType::New();
		writer->SetInput(mask);
		writer->SetFileName(outFileName);

		try {
			writer->Update();
		}
		catch (itk::ExceptionObject & e) {
			return false;
		}


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