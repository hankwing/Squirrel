/* 
 * File:   Partition.cpp
 * Author: xy
 * 
 * Created on October 18, 2013, 8:47 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_set>
#include <math.h>
#include <time.h>
#include <utility>
#include <string>
#include "Partition.h"
#include <unistd.h>
#include "StarFile.h"

using namespace std;

int count = 0;

Partition::Partition() {
	fieldWidth = 0;
	fieldHeight = 0;
}

Partition::Partition(const Partition& orig) {
	fieldWidth = 0;
	fieldHeight = 0;
}

Partition::Partition(float errBox, float minZoneLen, float searchRds) {
	errRadius = errBox;
	minZoneLength = minZoneLen;
	searchRadius = searchRds;
	fieldWidth = 0;
	fieldHeight = 0;
}

Partition::~Partition() {
	freeZoneArray();
}

void Partition::partitonStarField(StarFile *starFile) {

	totalStar = starFile->starNum;
	CMStar *starList = starFile->starList;

	if (fieldWidth == 0 || fieldHeight == 0) {
		getMinMaxXY(starList);
	}

	float zoneLength = sqrt(fieldWidth * fieldHeight / totalStar);
	if (zoneLength < minZoneLength)
		zoneLength = minZoneLength;
	zoneInterval = zoneLength;
	zoneIntervalRecp = 1 / zoneInterval;
	zoneXnum = ceil(fieldWidth * zoneIntervalRecp);
	zoneYnum = ceil(fieldHeight * zoneIntervalRecp);

	totalZone = zoneXnum * zoneYnum;
	zoneArray = (CMZone *) malloc(sizeof(CMZone) * totalZone);

	for (int i = 0; i < totalZone; i++) {
		zoneArray[i].star = NULL;
		zoneArray[i].starNum = 0;
	}

	CMStar *listHead = starList;
	CMStar *tmp = listHead;
	int tNum = 0;
	while (tmp) {
		listHead = tmp->next; //把tmp点从数据表中移除
		long idx = getZoneIndex(tmp); //获得tmp所属的树枝位置
		addStarToZone(tmp, idx); //把tmp点加入到树干的对应树枝中
		tmp = listHead; //取下一个点
		tNum++;
	}

#ifdef PRINT_CM_DETAIL
	printf("total add star %d\n", tNum);
	printf("the detail partition info is output to partitionDetail.cat\n");
	printZoneDetail("partitionDetail.cat");
#endif
}

/**
 * find the matched star of 'objStar'
 * @param objStar
 * @return the matched star is stored on objStar->match, 
 *         the distance between two stars is stored on objStar->error
 */
std::pair<int, acl::string> Partition::getMatchStar(CMStar *objStar, int outputFile,
		vector<vector<string>> offSetsList, int stepsNow) {

	std::pair<int, acl::string> matchedInfo(-1, "");
	long sZoneNum = 0;
	long *searchZonesIdx = getStarSearchZone(objStar, sZoneNum);

	float minError = errRadius;
	CMStar *minPoint = NULL;

	int i;
	for (i = 0; i < sZoneNum; i++) {
		CMStar *tmpPoint = searchSimilarStar(searchZonesIdx[i], objStar);
		if (tmpPoint != NULL && objStar->error < minError) {
			minError = objStar->error;
			minPoint = tmpPoint;
		}
	}
	if (minPoint) {
		// find the match star
		matchedInfo.first = minPoint->starId;
		objStar->match = minPoint;
		objStar->error = minError;

		acl::string info;

		objStar->toString(minPoint, info);
		matchedInfo.second = info;

		// add offSets by hand
		if( minPoint->starId % 5000 == 0) {
			int index = minPoint->starId / 5000 / 6;
			objStar->mag += atof(offSetsList[index][stepsNow].c_str()) ;
		}

		// construct json
		//starInfoJson[std::to_string(minPoint->starId)] = objStar->mag;
		// write matched start to file for abnormal detection
		std::stringstream ss;
		ss << minPoint->pixx << " " << minPoint->pixy << " " << minPoint->alpha << " " << minPoint->delta << " "
				<< minPoint->zone << " " << minPoint->starId << " " << objStar->mag << " " << minPoint->time <<
				" " << minPoint->ellipticity << " " << minPoint->ccdNum << "\n";
		// write the matched info to namedpipe for detecting abnormal behaviour
		if(outputFile != -1 && write(outputFile, ss.str().c_str(), ss.str().length()) < 0);
		//objStar->starFile->redisStrings.push_back(string);

	} else {
		objStar->match = NULL;
		objStar->error = errRadius;
	}
	free(searchZonesIdx);

	return matchedInfo;
}

/**
 * find the nearest matched star of 'objStar' in zone 'zoneIdx'
 * @param zoneIdx
 * @param objStar
 * @return matched star, the distance between two stars is stored on objStar->error
 */
CMStar *Partition::searchSimilarStar(long zoneIdx, CMStar *objStar) {

	float error = errRadius;
	CMStar *goalStar = NULL;

	CMStar *nextStar = zoneArray[zoneIdx].star;
	while (nextStar) {
		float distance = getLineDistance(nextStar, objStar);
		if (distance < error) {
			goalStar = nextStar;
			error = distance;
		}
		nextStar = nextStar->next;
	}
	objStar->error = error;

	return goalStar;
}

long Partition::getZoneIndex(CMStar * star) {

	long x = (long) ((star->pixx - minx) * zoneIntervalRecp);
	long y = (long) ((star->pixy - miny) * zoneIntervalRecp);

	return y * zoneXnum + x;
}

void Partition::addStarToZone(CMStar *star, long zoneIdx) {

	CMZone *zone = &zoneArray[zoneIdx];
	if (NULL == zone->star) {
		zone->star = star;
		star->next = NULL;
	} else {
		star->next = zone->star;
		zone->star = star;
	}
	zone->starNum = zone->starNum + 1;
}

void Partition::getMinMaxXY(CMStar *starList) {

	minx = starList->pixx;
	maxx = starList->pixx;
	miny = starList->pixy;
	maxy = starList->pixy;

	CMStar *nextStar = starList->next;
	while (nextStar) {
		if (nextStar->pixx > maxx) {
			maxx = nextStar->pixx;
		} else if (nextStar->pixx < minx) {
			minx = nextStar->pixx;
		}
		if (nextStar->pixy > maxy) {
			maxy = nextStar->pixy;
		} else if (nextStar->pixy < miny) {
			miny = nextStar->pixy;
		}
		nextStar = nextStar->next;
	}

	maxx = maxx + 1;
	maxy = maxy + 1;
	minx = minx - 1;
	miny = miny - 1;
	minx = minx < 0 ? 0 : minx;
	miny = miny < 0 ? 0 : miny;


	fieldWidth = maxx - minx;
	fieldHeight = maxy - miny;
}

long *Partition::getStarSearchZone(CMStar *star, long &sZoneNum) {

	float x = star->pixx - minx;
	float y = star->pixy - miny;

	int up = (y + searchRadius) * zoneIntervalRecp;
	int down = (y - searchRadius) * zoneIntervalRecp;
	int right = (x + searchRadius) * zoneIntervalRecp;
	int left = (x - searchRadius) * zoneIntervalRecp;

	if (up >= zoneYnum) {
		up = zoneYnum - 1;
	} else if (up < 0) {
		up = 0;
	}
	if (down >= zoneYnum) {
		down = zoneYnum - 1;
	} else if (down < 0) {
		down = 0;
	}
	if (right >= zoneXnum) {
		right = zoneXnum - 1;
	} else if (right < 0) {
		right = 0;
	}
	if (left >= zoneXnum) {
		left = zoneXnum - 1;
	} else if (left < 0) {
		left = 0;
	}

	int height = up - down + 1;
	int width = right - left + 1;
	sZoneNum = height * width;

	long *tIndex = (long *) malloc(sZoneNum * sizeof(long));
	int baseIndex = down * zoneXnum + left;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			tIndex[i * width + j] = i * zoneXnum + j + baseIndex;
		}
	}

	return tIndex;
}

void Partition::setSearchRadius(float searchRadius) {
	this->searchRadius = searchRadius;
}

float Partition::getSearchRadius() const {
	return searchRadius;
}

void Partition::setErrRadius(float errRadius) {
	this->errRadius = errRadius;
}

float Partition::getErrRadius() const {
	return errRadius;
}

void Partition::setMinZoneLength(float minZoneLength) {
	this->minZoneLength = minZoneLength;
}

float Partition::getMinZoneLength() const {
	return minZoneLength;
}

void Partition::setFieldHeight(float fieldHeight) {
	this->fieldHeight = fieldHeight;
}

void Partition::setFieldWidth(float fieldWidth) {
	this->fieldWidth = fieldWidth;
}

float Partition::getFieldHeight() const {
	return fieldHeight;
}

float Partition::getFieldWidth() const {
	return fieldWidth;
}

void Partition::printZoneDetail(char *fName) {

	FILE *fp;
	if ((fp = fopen(fName, "w")) == NULL) {
		printf("open file error!!\n");
		return;
	}

	fprintf(fp, "min x: %f\n", minx);
	fprintf(fp, "min y: %f\n", miny);
	fprintf(fp, "max x: %f\n", maxx);
	fprintf(fp, "max y: %f\n", maxy);
	fprintf(fp, "zone length: %f\n", zoneInterval);
	fprintf(fp, "zone width: %d\n", zoneXnum);
	fprintf(fp, "zone height: %d\n", zoneYnum);
	fprintf(fp, "total zones: %d\n", totalZone);
	fprintf(fp, "total stars: %d\n", totalStar);

	fprintf(fp, "\nZone details:\n");
	fprintf(fp, "\nzoneId\txId\tyId\tstarNum\tX...\n");

	int i = 0;
	int j = 0;
	for (i = 0; i < totalZone; i++) {

		if (zoneArray[i].starNum > 0) {
			j++;
			fprintf(fp, "%8d%5d%5d%8d", i + 1, i % zoneXnum, i / zoneXnum,
					zoneArray[i].starNum);
			CMStar *tmp = zoneArray[i].star;
			while (tmp) {
				fprintf(fp, "%15.8f", tmp->pixx);
				tmp = tmp->next;
			}
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
}

void Partition::freeZoneArray() {

	if (NULL != zoneArray) {
		for (int i = 0; i < totalZone; i++) {
			freeStarList(zoneArray[i].star);
		}
		free(zoneArray);
	}
}

void Partition::freeStarList(CMStar *starList) {

	if (NULL != starList) {
		CMStar *tStar = starList->next;
		while (tStar) {
			starList->next = tStar->next;
			if (NULL != tStar->line)
				free(tStar->line);
			free(tStar);
			tStar = starList->next;
		}
		if (NULL != starList->line)
			free(starList->line);
		free(starList);
	}
}
