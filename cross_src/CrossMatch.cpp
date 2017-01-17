/* 
 * File:   CrossMatch.cpp
 * Author: xy
 * 
 * Created on October 18, 2013, 8:48 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "StarFileFits.h"
#include "acl_cpp/lib_acl.hpp"
#include "CrossMatch.h"

typedef struct parameters {
	acl::redis_client_cluster *cluster;
	std::vector<std::vector<acl::string> > *sendList;
	StarFileFits* fitsFile;
	int lo;
	int hi;
} parameters;

CrossMatch::CrossMatch() {

  refStarFile = NULL;
  objStarFile = NULL;
  refStarFileNoPtn = NULL;
  objStarFileNoPtn = NULL;
  zones = NULL;
  fieldWidth = 0;
  fieldHeight = 0;
}

CrossMatch::CrossMatch(const CrossMatch& orig) {
  fieldWidth = 0;
  fieldHeight = 0;
}

CrossMatch::CrossMatch(StarFile* refStarFile, StarFile* objStarFile) {
	errRadius = 0;
	refStarFileNoPtn = NULL;
	objStarFileNoPtn = NULL;
	zones = NULL;
	this->fieldHeight = 0;
	this->fieldWidth = 0;
	this->refStarFile = refStarFile;
	this->objStarFile = objStarFile;
}

CrossMatch::~CrossMatch() {
  freeAllMemory();
}

void CrossMatch::match(char *refName, char *objName, float errorBox) {

  refStarFile = new StarFile();
  refStarFile->readStar(refName);
  objStarFile = new StarFile();
  objStarFile->readStar(objName);

  match(refStarFile, objStarFile, errorBox);
}

void CrossMatch::match(StarFile *refStarFile, StarFile *objStarFile, float errorBox) {

  //refStarFile->starList = NULL;

  CMStar *nextStar = objStarFile->starList;
  while (nextStar) {
      // core code!!
    zones->getMatchStar(nextStar);
    nextStar = nextStar->next;
  }

#ifdef PRINT_CM_DETAIL
  printf("partition match done!\n");
#endif
}

void CrossMatch::match(StarFile *refStarFile, StarFile *objStarFile,Partition * zones, float errorBox) {

  //refStarFile->starList = NULL;

  CMStar *nextStar = objStarFile->starList;
  while (nextStar) {
      // core code!!
	  std::pair<int, acl::string> matchResult = zones->getMatchStar(nextStar);
    if(matchResult.first == -1) {

    	objStarFile->OTStarCount ++;
    } else {
    	refStarFile->starDataCache[matchResult.first].push_back(matchResult.second);
    	objStarFile->matchedCount ++;
    }
    if( nextStar->mag >= 50) objStarFile->abStar ++;
    nextStar = nextStar->next;
  }

#ifdef PRINT_CM_DETAIL
  printf("partition match done!\n");
#endif
}

void decompressStarData( StarFileFits* refStarFile, acl::string compressed) {
	int keyIndex = compressed.find(' ');
	acl::string key;
	compressed.substr(key, 0, keyIndex);


}

void compressStarData( StarFileFits* refStarFile, std::vector<acl::string>& data, std::string key) {

	char *string_ptr=NULL;
	char * p;
	for( unsigned int k = 0; k < data.size(); k ++) {
		std::ostringstream ostr;	// whole compressed info
		std::ostringstream osHeader;
		std::bitset<32> header(0);

		for( int m = 0; m < 25 ; m++) {
			if( m == 0) {
				p = strtok_r(data[k].c_str(), " ", &string_ptr);
				// skip key value
				continue;
			}
			else {
				p = strtok_r(NULL, " ", &string_ptr);
			}

			float tValue = atof(p);
			float kValue = refStarFile->templateValues[key][m -1];
			if( tValue == kValue) header[m-1] = 0;
			else {
				header[m-1] = 1;
				std::ostringstream temp;
				temp << tValue - kValue;
				std::string changeValue = temp.str();
				int bitsValue = 0;
				for( unsigned int j = 0; j < changeValue.length(); j+=2) {
					if( changeValue[j] == '.') bitsValue = 10 << 4;
					else if( changeValue[j] == '-') bitsValue = 11 << 4;
					else bitsValue = (changeValue[j] - '0') << 4;
					if( j + 1 < changeValue.length()) {
						if( changeValue[j + 1] == '.') bitsValue = bitsValue + 10;
						else if( changeValue[j + 1] == '-') bitsValue = bitsValue + 11;
						else bitsValue = bitsValue + changeValue[j + 1] - '0';
					}
					ostr << static_cast<char>(bitsValue);
				}
				ostr << " ";
			}

		}
		// compress head
		for( int i = 0; i < 4; i++) {
			int start = i * 8;
			char value = 0;
			for( int j = start; j < start + 8; j++) {
				value = value * 10 + header[j];
			}
			osHeader << value;
		}

		acl::string newValue;
		newValue.append(key.c_str());
		newValue.append(" ");
		newValue.append(osHeader.str().c_str());
		newValue.append(" ");
		newValue.append(ostr.str().c_str());
		data[k] = newValue;

	}

}

static void* singleSendThread( void * arg) {
	// get key and value
	parameters *pms = (parameters*) arg;
	int failedTime = 0;
	int lo = pms->lo;
	int hi = pms->hi;
	std::vector<std::vector<acl::string> > *sendList = pms->sendList;

	acl::redis cmd;
	cmd.set_cluster(pms->cluster, 1000);
	for( int i = lo; i < hi; i++) {
		if( sendList->at(i).size() == 0) {
			// if i is null
			continue;
		}

		failedTime = 0;

		int keyIndex = sendList->at(i)[0].find(' ');
		acl::string key;
	    sendList->at(i)[0].substr(key, 0, keyIndex);
		//std::string value = sendString.substr(keyIndex+1);

	    compressStarData( pms->fitsFile, sendList->at(i), key.c_str());
		while( cmd.rpush(key.c_str(), sendList->at(i)) < 0) {
			printf("insert failed because %s %d\n", cmd.result_error(), i);
			if( failedTime ++ > 5) {
				pms->cluster->set(pms->fitsFile->redisHost, 10, 10, 1000);
				cmd.set_cluster(pms->cluster, 1000);
				sleep(3);
			}
		}

		//acl::string result;
		//cmd.rpop(key.c_str(), result);
		//printf("result string length: %d\n", result.length());

		sendList->at(i).clear();
		//cmd.rpush(key.c_str(), value.c_str(), NULL);
		cmd.clear();

	}

	return NULL;

}

/**
 * send matched results to redis using multiple threads
 */
void CrossMatch::sendResultsToRedis(acl::redis_client_cluster *cluster,
		StarFileFits *refStars, int usedThreads, int& control) {
	pthread_attr_t attrs[usedThreads];
	pthread_t ids[usedThreads];
	parameters pms[usedThreads];
	int steps = refStars->starDataCache.size() / usedThreads;
	int start = control * steps + 1;
	control = (control + 1) % usedThreads;
	int end = start + steps;
	steps = ( end - start) / usedThreads;

	int i = 0;

	for( i = 0; i <  usedThreads - 1; i++) {
		pms[i].fitsFile = refStars;
		pms[i].cluster = cluster;
		pms[i].sendList = &refStars->starDataCache;
		pms[i].lo = start + i * steps;
		pms[i].hi = pms[i].lo + steps;

		pthread_attr_init(&attrs[i]);

		// create first thread
		pthread_create(&ids[i], &attrs[i], singleSendThread, &pms[i]);

	}

	pms[i].fitsFile = refStars;
	pms[i].cluster = cluster;
	pms[i].sendList = &refStars->starDataCache;
	pms[i].lo = start + i * steps;
	pms[i].hi = end;
	pthread_attr_init(&attrs[i]);
	// create first thread
	pthread_create(&ids[i], &attrs[i], singleSendThread, &pms[i]);

	// wait for all threads stopping
	for( i = 0; i < usedThreads; i++) {
		pthread_join(ids[i], NULL);
	}

}

/**
 * circulate each star on 'refList', find the nearest on as the match star of objStar
 * the matched star is stored on obj->match, 
 * the distance between two stars is stored on obj->error
 * @param ref
 * @param obj
 */
void CrossMatch::matchNoPartition(char *refName, char *objName, float errorBox) {

  refStarFileNoPtn = new StarFile();
  refStarFileNoPtn->readStar(refName);
  objStarFileNoPtn = new StarFile();
  objStarFileNoPtn->readStar(objName);

  matchNoPartition(refStarFileNoPtn, objStarFileNoPtn, errorBox);
}

/**
 * the matched star is stored on obj->match, 
 * the distance between two stars is stored on obj->error
 * @param ref
 * @param obj
 */
void CrossMatch::matchNoPartition(StarFile *refStarFileNoPtn, StarFile *objStarFileNoPtn, float errorBox) {

  CMStar *tObj = objStarFileNoPtn->starList;

  while (tObj) {
    CMStar *tRef = refStarFileNoPtn->starList;
    float tError = getLineDistance(tRef, tObj);
    tObj->match = tRef;
    tObj->error = tError;
    tRef = tRef->next;
    while (tRef) {
      tError = getLineDistance(tRef, tObj);
      if (tError < tObj->error) {
        tObj->match = tRef;
        tObj->error = tError;
      }
      tRef = tRef->next;
    }
    tObj = tObj->next;
  }

#ifdef PRINT_CM_DETAIL
  printf("no partition match done!\n");
#endif
}

void CrossMatch::freeAllMemory() {

  if (NULL != refStarFile)
    delete refStarFile;
  if (NULL != objStarFile)
    delete objStarFile;
  if (NULL != refStarFileNoPtn)
    delete refStarFileNoPtn;
  if (NULL != objStarFileNoPtn)
    delete objStarFileNoPtn;
  if (NULL != zones) {
    delete zones;
  }
}

void CrossMatch::compareResult(char *refName, char *objName, char *outfName, float errorBox) {

  match(refName, objName, errorBox);
  matchNoPartition(refName, objName, errorBox);
  compareResult(objStarFile, objStarFileNoPtn, outfName, errorBox);
}

void CrossMatch::compareResult(StarFile *objStarFile, StarFile *objStarFileNoPtn, const char *outfName, float errorBox) {

  if (NULL == objStarFile || NULL == objStarFileNoPtn) {
    printf("StarFile is null\n");
    return;
  }

  FILE *fp = fopen(outfName, "w");

  CMStar *tStar1 = objStarFile->starList;
  CMStar *tStar2 = objStarFileNoPtn->starList;
  int i = 0, j = 0, k = 0, m = 0, n = 0, g = 0;
  while (NULL != tStar1 && NULL != tStar2) {
    if (NULL != tStar1->match && NULL != tStar2->match) {
      i++;
      float errDiff = fabs(tStar1->error - tStar2->error);
      if (errDiff < CompareFloat)
        n++;
    } else if (NULL != tStar1->match && NULL == tStar2->match) {
      j++;
    } else if (NULL == tStar1->match && NULL != tStar2->match) {//ommit and OT
      k++;
      if (tStar2->error < errorBox)
        g++;
    } else {
      m++;
    }
    tStar1 = tStar1->next;
    tStar2 = tStar2->next;
  }
  fprintf(fp, "total star %d\n", i + j + k + m);
  fprintf(fp, "matched %d , two method same %d\n", i, n);
  fprintf(fp, "partition matched but nopartition notmatched %d\n", j);
  fprintf(fp, "nopartition matched but partition notmatched %d, small than errorBox %d\n", k, g);
  fprintf(fp, "two method are not matched %d\n", m);

  fprintf(fp, "\nX1,Y1,X1m,Y1m,err1 is the partition related info\n");
  fprintf(fp, "X2,Y2,X2m,Y2m,err2 is the nopartition related info\n");
  fprintf(fp, "X1,Y1,X2,Y2 is orig X and Y position of stars\n");
  fprintf(fp, "X1m,Y1m,X2m,Y2m is matched X and Y position of stars\n");
  fprintf(fp, "pos1,pos2 is the two method's match distance\n");
  fprintf(fp, "the following list is leaked star of partition method, total %d\n", g);
  fprintf(fp, "X1\tY1\tX2\tY2\tX1m\tY1m\tX2m\tY2m\tpos1\tpos2\n");
  tStar1 = objStarFile->starList;
  tStar2 = objStarFileNoPtn->starList;
  while (NULL != tStar1 && NULL != tStar2) {
    if (NULL == tStar1->match && NULL != tStar2->match && tStar2->error < errorBox) { //ommit and OT
      fprintf(fp, "%12f %12f %12f %12f %12f %12f %12f %12f %12f %12f\n",
              tStar1->pixx, tStar1->pixy, tStar2->pixx, tStar2->pixy,
              0.0, 0.0, tStar2->match->pixx, tStar2->match->pixy,
              tStar1->error, tStar2->error);
    }
    tStar1 = tStar1->next;
    tStar2 = tStar2->next;
  }

  fprintf(fp, "the following list is OT\n");
  fprintf(fp, "X1\tY1\tX2\tY2\tX1m\tY1m\tX2m\tY2m\tpos1\tpos2, total %d\n", k - g);
  tStar1 = objStarFile->starList;
  tStar2 = objStarFileNoPtn->starList;
  while (NULL != tStar1 && NULL != tStar2) {
    if (NULL == tStar1->match && NULL != tStar2->match && tStar2->error > errorBox) { //ommit and OT
      fprintf(fp, "%12f %12f %12f %12f %12f %12f %12f %12f %12f %12f\n",
              tStar1->pixx, tStar1->pixy, tStar2->pixx, tStar2->pixy,
              0.0, 0.0, tStar2->match->pixx, tStar2->match->pixy,
              tStar1->error, tStar2->error);
    }
    tStar1 = tStar1->next;
    tStar2 = tStar2->next;
  }

  fclose(fp);
}

void CrossMatch::printMatchedRst(char *outfName, float errorBox) {

  FILE *fp = fopen(outfName, "w");
  fprintf(fp, "Id\tX\tY\tmId\tmX\tmY\tdistance\n");

  long count = 0;
  CMStar *tStar = objStarFile->starList;
  while (NULL != tStar) {
    if (NULL != tStar->match && tStar->error < errorBox) {
      fprintf(fp, "%8d %12f %12f %8d %12f %12f %12f\n",
              tStar->starId, tStar->pixx, tStar->pixy, tStar->match->starId,
              tStar->match->pixx, tStar->match->pixy, tStar->error);
      count++;
    }
    tStar = tStar->next;
  }
  fclose(fp);

#ifdef PRINT_CM_DETAIL
  printf("matched stars %d\n", count);
#endif
}

void CrossMatch::printMatchedRst(char *outfName, StarFile *starFile, float errorBox) {

  FILE *fp = fopen(outfName, "w");
  fprintf(fp, "Id\tX\tY\tmId\tmX\tmY\tdistance\n");

  long count = 0;
  CMStar *tStar = starFile->starList;
  while (NULL != tStar) {
    if (NULL != tStar->match && tStar->error < errorBox) {
        // print match results to alluxio file
      fprintf(fp, "%8d %12f %12f %8d %12f %12f %12f\n",
              tStar->starId, tStar->pixx, tStar->pixy, tStar->match->starId,
              tStar->match->pixx, tStar->match->pixy, tStar->error);
      count++;
    }
    tStar = tStar->next;
  }
  fclose(fp);

#ifdef PRINT_CM_DETAIL
  printf("matched stars %d\n", count);
#endif
}

void CrossMatch::printAllStarList(char *outfName, StarFile *starFile, float errorBox) {

  FILE *fp = fopen(outfName, "w");
  fprintf(fp, "Id\tX\tY\tmId\tmX\tmY\tdistance\n");

  long count = 0;
  CMStar *tStar = starFile->starList;
  while (NULL != tStar) {
    if (NULL != tStar->match) {
      fprintf(fp, "%8d %12f %12f %8d %12f %12f %12f\n",
              tStar->starId, tStar->pixx, tStar->pixy, tStar->match->starId,
              tStar->match->pixx, tStar->match->pixy, tStar->error);
    } else {
      fprintf(fp, "%8d %12f %12f %8d %12f %12f %12f\n",
              tStar->starId, tStar->pixx, tStar->pixy, 0, 0.0, 0.0, tStar->error);
    }
    count++;
    tStar = tStar->next;
  }
  fclose(fp);

#ifdef PRINT_CM_DETAIL
  printf("matched stars %d\n", count);
#endif
}

void CrossMatch::printOTStar(char *outfName, float errorBox) {

  FILE *fp = fopen(outfName, "w");
  fprintf(fp, "Id\tX\tY\n");

  long count = 0;
  CMStar *tStar = objStarFile->starList;
  while (NULL != tStar) {
    if (NULL == tStar->match) {
      fprintf(fp, "%8d %12f %12f\n",
              tStar->starId, tStar->pixx, tStar->pixy);
      count++;
    }
    tStar = tStar->next;
  }
  fclose(fp);

#ifdef PRINT_CM_DETAIL
  printf("OT stars %d\n", count);
#endif
}

void CrossMatch::testCrossMatch() {

  char refName[30] = "data/referance.cat";
  char objName[30] = "data/object.cat";
  char matchedName[30] = "data/matched.cat";
  char otName[30] = "data/ot.cat";
  float errorBox = 0.7;

  CrossMatch *cm = new CrossMatch();
  cm->match(refName, objName, errorBox);
  cm->printMatchedRst(matchedName, errorBox);
  cm->printOTStar(otName, errorBox);
  cm->freeAllMemory();

}

void CrossMatch::partitionAndNoPartitionCompare() {

  char refName[30] = "data/referance.cat";
  char objName[30] = "data/object.cat";
  char cmpOutName[30] = "data/cmpOut.cat";
  float errorBox = 0.7;

  CrossMatch *cm = new CrossMatch();
  cm->compareResult(refName, objName, cmpOutName, errorBox);
  cm->freeAllMemory();

}

void CrossMatch::setFieldHeight(float fieldHeight) {
  this->fieldHeight = fieldHeight;
}

void CrossMatch::setFieldWidth(float fieldWidth) {
  this->fieldWidth = fieldWidth;
}
