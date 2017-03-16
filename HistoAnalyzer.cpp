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

bool HistoAnalyzer::ReadSlidesDicoms() {

	return true; 

}