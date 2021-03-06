#include "HistoAnalyzer.h"

HistoAnalyzer::HistoAnalyzer(std::string indir, std::string dbpath, std::string maskpath, std::string outdir):
	indir(indir),
	dbpath(dbpath),
	maskpath(maskpath),
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
		"ParameterMapName = 'threetimepoint' OR "
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
		"ParameterMapName = 'r_nlaif_c' OR "
		"ParameterMapName = 'threetimepoint_c';";


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
	HistoAnalyzer::ImageType::Pointer prosmask;

	try {
		 img = HistoAnalyzer::ReadImageMap(mappaths[0]);
	}
	catch (itk::ExceptionObject & e) {
		std::cerr << e.GetDescription() << std::endl;
		return false;
	}

	try {
		prosmask = HistoAnalyzer::ReadImageMap(maskpath);
	}
	catch (itk::ExceptionObject & e) {
		std::cerr << e.GetDescription() << std::endl;
		return false;
	}

	//allocate histology mask
	typedef itk::Vector<int, 4> HistVoxelType;
	typedef itk::Image<HistVoxelType, 3> HistImageType;
	HistVoxelType v00;
	v00.Fill(0);
	HistImageType::Pointer histmask = HistImageType::New();
	histmask->SetRegions(img->GetLargestPossibleRegion());
	histmask->Allocate();
	histmask->SetOrigin(img->GetOrigin());
	histmask->SetDirection(img->GetDirection());
	histmask->SetSpacing(img->GetSpacing());
	histmask->FillBuffer(v00);
	histmask->Update();

	//allocate pathology mask
	HistoAnalyzer::ImageType::Pointer pathmask = HistoAnalyzer::ImageType::New();
	pathmask->SetRegions(img->GetLargestPossibleRegion());
	pathmask->Allocate();
	pathmask->SetOrigin(img->GetOrigin());
	pathmask->SetDirection(img->GetDirection());
	pathmask->SetSpacing(img->GetSpacing());
	pathmask->FillBuffer(0.0);
	pathmask->Update();
	
	//allocate mask
	mask = HistoAnalyzer::ImageType::New();
	mask->SetRegions(img->GetLargestPossibleRegion());
	mask->Allocate();
	mask->SetOrigin(img->GetOrigin());
	mask->SetDirection(img->GetDirection());
	mask->SetSpacing(img->GetSpacing());
	mask->FillBuffer(0.0);
	mask->Update();


	//transform file
	std::string tfmdir = indir + "\\LinearTransform.tfm";
	typedef itk::AffineTransform<double, 3> TransformType;
	itk::TransformFileReader::Pointer tfmreader = itk::TransformFileReader::New();

	tfmreader->SetFileName(tfmdir);

	try {
		tfmreader->Update();
	}
	catch (itk::ExceptionObject &e) {
		std::cerr << e.GetDescription() << std::endl;
	}

	const itk::TransformFileReader::TransformListType * transforms = tfmreader->GetTransformList();
	TransformType::Pointer tfm = static_cast<TransformType*>((*transforms->begin()).GetPointer());
	itk::Transform<double, 3, 3>::Pointer tfm2 = tfm->GetInverseTransform();

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

	//apply transform
	typedef itk::ResampleImageFilter<RGBImageType, RGBImageType> FilterType;
	FilterType::Pointer filter;

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
			std::cerr << e.GetDescription() << std::endl;
			return false;
		}
		
		img_s = reader->GetOutput();
		//RGBImageType::SizeType rfsize = img_s->GetLargestPossibleRegion().GetSize();

		//filter = FilterType::New();
		//filter->SetInput(img_s);
		//filter->SetSize(rfsize);
		//filter->SetOutputParametersFromImage(img_s);
		////filter->SetOutputSpacing(img_s->GetSpacing());
		////filter->SetOutputDirection(img_s->GetDirection());
		////filter->SetOutputOrigin(img_s->GetOrigin());
		//filter->SetTransform(tfm);

		//try {
		//	filter->Update();
		//}
		//catch (itk::ExceptionObject & e) {
		//	std::cerr << e.GetDescription() << std::endl;
		//	return false;
		//}

		//img_s = filter->GetOutput();

		//loop through index values
		RGBImageType::RegionType rgbregion = img_s->GetLargestPossibleRegion();
		itk::ImageRegionIterator<RGBImageType> rgbimageiterator(img_s, rgbregion);

		PixelType v;
		HistVoxelType hv;
		RGBImageType::IndexType ind0;
		HistoAnalyzer::ImageType::IndexType ind1;
		RGBImageType::PointType p, pt;
		RGBImageType::SizeType s = rgbregion.GetSize();
		while (!rgbimageiterator.IsAtEnd())
		{

			v = rgbimageiterator.Get();
			ind0 = rgbimageiterator.GetIndex();
			img_s->TransformIndexToPhysicalPoint(ind0, p);
			pt = tfm2->TransformPoint(p);
			histmask->TransformPhysicalPointToIndex(pt, ind1);
			hv = histmask->GetPixel(ind1);

			if(ind0[1] < (s[1]-60)) {
				if (v[0] == 0 && v[1] == 128 && v[2] == 0) {
					if (img->GetPixel(ind1) > -1.99) {
						hv[1]++;
						histmask->SetPixel(ind1, hv);
					}
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
					if (img->GetPixel(ind1) > -1.99) {
						hv[2]++;
						histmask->SetPixel(ind1, hv);
					}
				}
				else if (v[0] == 128 && v[1] == 128 && v[2] == 128) {
					if (img->GetPixel(ind1) > -1.99) {
						hv[3]++;
						histmask->SetPixel(ind1, hv);
					}
				}
				else {
					if (prosmask->GetPixel(ind1) == 1) {
						hv[0]++;
						histmask->SetPixel(ind1, hv);
					}
				}
			}

			++rgbimageiterator;

		}

	}

	//loop through mask/pathmask
	HistImageType::RegionType histregion = histmask->GetLargestPossibleRegion();
	itk::ImageRegionIterator<HistImageType> histiterator(histmask, histregion);
	HistVoxelType hv;
	while (!histiterator.IsAtEnd())
	{
		hv = histiterator.Get();

		if (hv[0] > 0 && hv[1] == 0 && hv[2] == 0 && hv[3] == 0) {
			mask->SetPixel(histiterator.GetIndex(), 1);
		} else if (hv[1] > 1 && hv[1] > hv[2] && hv[1] > hv[3]) {
			mask->SetPixel(histiterator.GetIndex(), 3);
			pathmask->SetPixel(histiterator.GetIndex(), 1);
		}
		else if (hv[2] > 1 && hv[2] > hv[1] && hv[2] > hv[3]) {
			mask->SetPixel(histiterator.GetIndex(), 4);
			pathmask->SetPixel(histiterator.GetIndex(), 1);
		}
		else if (hv[3] > 1 && hv[3] > hv[2] && hv[3] > hv[1]) {
			mask->SetPixel(histiterator.GetIndex(), 2);
			pathmask->SetPixel(histiterator.GetIndex(), 1);
		}

		++histiterator;

	}
	
	//dilate pathology by 2 mm
	HistoAnalyzer::ImageType::SizeType si;
	HistoAnalyzer::ImageType::SpacingType sp = mask->GetSpacing();
	si[0] = round(2 / sp[0]);
	si[1] = round(2 / sp[1]);
	si[2] = round(2 / sp[2]);

	//apply morphological expansion on mask
	typedef itk::BinaryBallStructuringElement<HistoAnalyzer::ImageType::PixelType, 3> StructuringElementType;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(si);
	structuringElement.CreateStructuringElement();

	typedef itk::BinaryDilateImageFilter <HistoAnalyzer::ImageType, HistoAnalyzer::ImageType, StructuringElementType> BinaryDilateImageFilterType;
	BinaryDilateImageFilterType::Pointer dilateFilter = BinaryDilateImageFilterType::New();

	dilateFilter->SetInput(pathmask);
	dilateFilter->SetKernel(structuringElement);
	dilateFilter->SetDilateValue(1);
	dilateFilter->Update();
	pathmask = dilateFilter->GetOutput();

	//loop through masks, elimating normal tissue labels within 2 mm of pathologies
	HistoAnalyzer::ImageType::RegionType maskregion = mask->GetLargestPossibleRegion();
	itk::ImageRegionIterator<HistoAnalyzer::ImageType> maskiterator(mask, maskregion);	
	int v0, v1;
	while (!maskiterator.IsAtEnd())
	{
		v0 = maskiterator.Get();
		v1 = pathmask->GetPixel(maskiterator.GetIndex());

		if (v0 == 1 && v1 == 1) {
			maskiterator.Set(0);
		}

		++maskiterator;

	}


	int v;
	maskiterator.GoToBegin();
	while (!maskiterator.IsAtEnd())
	{

		v = (int)maskiterator.Get();

		switch (v) {
		case 1:
			np.push_back(maskiterator.GetIndex());
			break;
		case 2:
			pin.push_back(maskiterator.GetIndex());
			break;
		case 3:
			g6.push_back(maskiterator.GetIndex());
			break;
		case 4:
			g7.push_back(maskiterator.GetIndex());
			break;
		}

		++maskiterator;

	}

	return true; 

}

bool HistoAnalyzer::WriteVoxelArrays() {

	//allocate memory to hold voxel values
	float minarea = 3.0;

	HistoAnalyzer::ImageType::SpacingType si = mask->GetSpacing();

	if (g6.size()*si[0] * si[1] > minarea) vg6 = (float*)calloc(g6.size(), sizeof(float));
	if (g7.size()*si[0] * si[1] > minarea) vg7 = (float*)calloc(g7.size(), sizeof(float));
	if (np.size()*si[0] * si[1] > minarea) vnp = (float*)calloc(np.size(), sizeof(float));
	if (pin.size()*si[0] * si[1] > minarea) vpin = (float*)calloc(pin.size(), sizeof(float));

	mg6 = (float*)calloc(mapnames.size(), sizeof(float));
	sg6 = (float*)calloc(mapnames.size(), sizeof(float));
	mg7 = (float*)calloc(mapnames.size(), sizeof(float));
	sg7 = (float*)calloc(mapnames.size(), sizeof(float));
	mnp = (float*)calloc(mapnames.size(), sizeof(float));
	snp = (float*)calloc(mapnames.size(), sizeof(float));
	mpin = (float*)calloc(mapnames.size(), sizeof(float));
	spin = (float*)calloc(mapnames.size(), sizeof(float));

	for (int i = 0; i < mapnames.size(); i++) {

		HistoAnalyzer::ImageType::Pointer img;
		if (strcmp(mapnames[i].c_str(), "threetimepoint_c") == 0 || strcmp(mapnames[i].c_str(), "threetimepoint") == 0) {
			 img = HistoAnalyzer::ReadImage3TP(mappaths[i]);
		}
		else {
			img = HistoAnalyzer::ReadImageMap(mappaths[i]);
		}

		

		if (g6.size()*si[0]*si[1] > minarea) {
			for (int j = 0; j < g6.size(); j++) {
				vg6[j] = img->GetPixel(g6[j]);
				//if (strcmp(mapnames[i].c_str(), "kep_nlrrm_c")==0 && vg6[j] < 0.0) {
				//	float v = vg6[j];
				//	HistoAnalyzer::ImageType::IndexType dbi = g6[j];
				//	std::cout << vg6[j] << std::endl;
				//}
			}
			mg6[i] = HistoAnalyzer::Mean<float>(vg6, g6.size());
			sg6[i] = HistoAnalyzer::STD<float>(vg6, g6.size(), mg6[i]);

			HistoAnalyzer::WriteVoxelsScalar(vg6, g6.size(), outdir + "\\" + mapnames[i] + "_g6.raw", outdir + "\\" + mapnames[i] + "_g6.txt");
		}
		else {
			mg6[i] = 0;
			sg6[i] = 0;
		}

		if (g7.size()*si[0] * si[1] > minarea) {
			for (int j = 0; j < g7.size(); j++) {
				vg7[j] = img->GetPixel(g7[j]);
				//if (strcmp(mapnames[i].c_str(), "kep_nlrrm_c") == 0 && vg7[j] < 0.0) {
				//	float v = vg7[j];
				//	HistoAnalyzer::ImageType::IndexType dbi = g7[j];
				//	std::cout << vg7[j] << std::endl;
				//}
			}
			mg7[i] = HistoAnalyzer::Mean<float>(vg7, g7.size());
			sg7[i] = HistoAnalyzer::STD<float>(vg7, g7.size(), mg7[i]);

			HistoAnalyzer::WriteVoxelsScalar(vg7, g7.size(), outdir + "\\" + mapnames[i] + "_g7.raw", outdir + "\\" + mapnames[i] + "_g7.txt");
		}
		else {
			mg7[i] = 0;
			sg7[i] = 0;
		}

		if (np.size()*si[0] * si[1] > minarea) {
			for (int j = 0; j < np.size(); j++) {
				vnp[j] = img->GetPixel(np[j]);
				//if (strcmp(mapnames[i].c_str(), "kep_nlrrm_c") == 0 && vnp[j] < 0.0) {
				//	float v = vnp[j];
				//	HistoAnalyzer::ImageType::IndexType dbi = np[j];
				//	std::cout << vnp[j] << std::endl;
				//}
			}
			mnp[i] = HistoAnalyzer::Mean<float>(vnp, np.size());
			snp[i] = HistoAnalyzer::STD<float>(vnp, np.size(), mnp[i]);

			HistoAnalyzer::WriteVoxelsScalar(vnp, np.size(), outdir + "\\" + mapnames[i] + "_np.raw", outdir + "\\" + mapnames[i] + "_np.txt");
		}
		else {
			mnp[i] = 0;
			snp[i] = 0;
		}

		if (pin.size()*si[0] * si[1] > minarea) {
			for (int j = 0; j < pin.size(); j++) {
				vpin[j] = img->GetPixel(pin[j]);
				//if (strcmp(mapnames[i].c_str(), "kep_nlrrm_c") == 0 && vpin[j] < 0.0) {
				//	float v = vpin[j];
				//	HistoAnalyzer::ImageType::IndexType dbi = pin[j];
				//	std::cout << pin[j] << std::endl;
				//}
			}
			mpin[i] = HistoAnalyzer::Mean<float>(vpin, pin.size());
			spin[i] = HistoAnalyzer::STD<float>(vpin, pin.size(), mpin[i]);

			HistoAnalyzer::WriteVoxelsScalar(vpin, pin.size(), outdir + "\\" + mapnames[i] + "_pin.raw", outdir + "\\" + mapnames[i] + "_pin.txt");
		}
		else {
			mpin[i] = 0;
			spin[i] = 0;
		}

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
		std::cerr << e.GetDescription() << std::endl;
		return false;
	}

	return true;
}

bool HistoAnalyzer::WriteDBMeanSD() {

	//open headerdata.db
	sqlite3 *db;
	char *errmsg;
	if (sqlite3_open(dbpath.c_str(), &db) != SQLITE_OK) {
		sqlite3_close(db);
		return false;
	}

	//create a temporalanalysis table
	if (sqlite3_exec(db, "DROP TABLE IF EXISTS histologyanalysis;", NULL, 0, &errmsg) != SQLITE_OK) {
		std::cerr << errmsg << std::endl;
		sqlite3_close(db);
		return  false;
	}

	char buffer0[] = "CREATE TABLE IF NOT EXISTS histologyanalysis ("
		"MapName VARCHAR(256), "
		"G6_VoxelPath VARCHAR(256), "
		"G7_VoxelPath VARCHAR(256), "
		"NP_VoxelPath VARCHAR(256), "
		"PIN_VoxelPath VARCHAR(256), "
		"G6_NumberVoxels INTEGER(16) DEFAULT 0, "
		"G7_NumberVoxels INTEGER(16) DEFAULT 0, "
		"NP_NumberVoxels INTEGER(16) DEFAULT 0, "
		"PIN_NumberVoxels INTEGER(16) DEFAULT 0, "
		"G6_Volume FLOAT DEFAULT 0.0, "
		"G7_Volume FLOAT DEFAULT 0.0, "
		"NP_Volume FLOAT DEFAULT 0.0, "
		"PIN_Volume FLOAT DEFAULT 0.0, "
		"G6_Mean FLOAT DEFAULT 0.0, "
		"G7_Mean FLOAT DEFAULT 0.0, "
		"NP_Mean FLOAT DEFAULT 0.0, "
		"PIN_Mean FLOAT DEFAULT 0.0, "
		"G6_SD FLOAT DEFAULT 0.0, "
		"G7_SD FLOAT DEFAULT 0.0, "
		"NP_SD FLOAT DEFAULT 0.0, "
		"PIN_SD FLOAT DEFAULT 0.0"
		");";

	if (sqlite3_exec(db, buffer0, NULL, 0, &errmsg) != SQLITE_OK) {
		std::cerr << errmsg << std::endl;
		sqlite3_close(db);
		return  false;
	}

	//insert
	if (sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::cout << errmsg << std::endl;
		sqlite3_close(db);
		return  false;
	}

	//prepare insert statement
	char buffer1[] = "INSERT INTO histologyanalysis ("
		"MapName, "
		"G6_VoxelPath, "
		"G7_VoxelPath, "
		"NP_VoxelPath, "
		"PIN_VoxelPath, "
		"G6_NumberVoxels, "
		"G7_NumberVoxels, "
		"NP_NumberVoxels, "
		"PIN_NumberVoxels, "
		"G6_Volume, "
		"G7_Volume, "
		"NP_Volume, "
		"PIN_Volume, "
		"G6_Mean, "
		"G7_Mean, "
		"NP_Mean, "
		"PIN_Mean, "
		"G6_SD, "
		"G7_SD, "
		"NP_SD, "
		"PIN_SD"
		") VALUES ("
		"?1, "
		"?2, "
		"?3, "
		"?4, "
		"?5, "
		"?6, "
		"?7, "
		"?8, "
		"?9, "
		"?10, "
		"?11, "
		"?12, "
		"?13, "
		"?14, "
		"?15, "
		"?16, "
		"?17, "
		"?18, "
		"?19, "
		"?20, "
		"?21"
		");";

	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db, buffer1, strlen(buffer1), &stmt, NULL) != SQLITE_OK) {
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return false;
	}

	//calculate voxel volumes
	HistoAnalyzer::ImageType::SpacingType s = mask->GetSpacing();

	//step
	for (int i = 0; i < mapnames.size(); i++) {

		std::string pg6 = outdir + "\\" + mapnames[i] + "_g6.raw";
		std::string pg7 = outdir + "\\" + mapnames[i] + "_g7.raw";
		std::string pnp = outdir + "\\" + mapnames[i] + "_np.raw";
		std::string ppin = outdir + "\\" + mapnames[i] + "_pin.raw";

		sqlite3_bind_text(stmt, 1, mapnames[i].c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, pg6.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, pg7.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, pnp.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 5, ppin.c_str(), -1, SQLITE_TRANSIENT);

		sqlite3_bind_int(stmt, 6, g6.size());
		sqlite3_bind_int(stmt, 7, g7.size());
		sqlite3_bind_int(stmt, 8, np.size());
		sqlite3_bind_int(stmt, 9, pin.size());

		sqlite3_bind_double(stmt, 10, g6.size()*s[0]*s[1]);
		sqlite3_bind_double(stmt, 11, g7.size()*s[0] * s[1]);
		sqlite3_bind_double(stmt, 12, np.size()*s[0] * s[1]);
		sqlite3_bind_double(stmt, 13, pin.size()*s[0] * s[1]);

		sqlite3_bind_double(stmt, 14, mg6[i]);
		sqlite3_bind_double(stmt, 15, mg7[i]);
		sqlite3_bind_double(stmt, 16, mnp[i]);
		sqlite3_bind_double(stmt, 17, mpin[i]);

		sqlite3_bind_double(stmt, 18, sg6[i]);
		sqlite3_bind_double(stmt, 19, sg7[i]);
		sqlite3_bind_double(stmt, 20, snp[i]);
		sqlite3_bind_double(stmt, 21, spin[i]);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			return false;
		}

		sqlite3_reset(stmt);

	}

	if (sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, &errmsg) != SQLITE_OK) {
		std::cout << errmsg << std::endl;
		sqlite3_close(db);
		return  false;
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return true;

}


HistoAnalyzer::ImageType::Pointer HistoAnalyzer::ReadImageMap(std::string mappath) {

	typedef itk::ImageFileReader<HistoAnalyzer::ImageType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();

	reader->SetFileName(mappath);
	reader->Update();

	return reader->GetOutput();

}

HistoAnalyzer::ImageType::Pointer HistoAnalyzer::ReadImage3TP(std::string mappath) {

	typedef itk::Vector<int, 3> VoxelType;
	typedef itk::Image<VoxelType, 3> RGBImageType;
	typedef itk::ImageFileReader<RGBImageType> ReaderType;
	ReaderType::Pointer reader = ReaderType::New();

	reader->SetFileName(mappath);
	reader->Update();

	RGBImageType::Pointer rgbimage = RGBImageType::New();
	rgbimage = reader->GetOutput();

	//convert from RGB to greyscale
	HistoAnalyzer::ImageType::Pointer img = HistoAnalyzer::ImageType::New();
	img->SetRegions(rgbimage->GetLargestPossibleRegion());
	img->Allocate();
	img->SetOrigin(rgbimage->GetOrigin());
	img->SetDirection(rgbimage->GetDirection());
	img->SetSpacing(rgbimage->GetSpacing());
	img->FillBuffer(0.0);
	img->Update();

	RGBImageType::RegionType rgbregion = rgbimage->GetLargestPossibleRegion();
	itk::ImageRegionIterator<RGBImageType> rgbimageiterator(rgbimage, rgbregion);

	while (rgbimageiterator != rgbimageiterator.End()) {

		VoxelType v = rgbimageiterator.Get();
		float f = 0.0;

		if (v[0] > 5) {
			f = ((float)v[0] / 255.0) + 2.0;
			img->SetPixel(rgbimageiterator.GetIndex(), f);
		}
		else if (v[1] > 5) {
			f = ((float)v[1] / 255.0) + 1.0;
			img->SetPixel(rgbimageiterator.GetIndex(), f);
		}
		else if (v[2] > 5) {
			f = (float)v[2] / 255.0;
			img->SetPixel(rgbimageiterator.GetIndex(), f);
		}

		++rgbimageiterator;
	}

	return img;

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

bool HistoAnalyzer::WriteVoxelsScalar(float * C, size_t n_v, std::string binaryfilename, std::string textfilename) {

	std::ofstream ofs;

	//text
	ofs.open(textfilename);
	if (ofs.is_open()) {
		ofs << "nvoxels: " << n_v << std::endl;
		ofs.close();
	}
	else {
		return false;
	}

	//binary
	ofs.open(binaryfilename, std::ios::binary);
	if (ofs.is_open()) {
		ofs.write(reinterpret_cast<char*>(C), n_v * sizeof(float));
		ofs.close();
	}
	else {
		return false;
	}

	return true;
}