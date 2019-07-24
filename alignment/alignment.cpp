//
// Created by notoraptor on 27/06/2019.
//

#include <algorithm>
#include <vector>
#include <iostream>
#include <cstring>
#include <thread>
#include <cmath>
#include <sstream>
#include "alignment.hpp"

double alignmentScore(std::vector<double>& matrix, const int* a, const int* b, int columns, double interval, int gapScore) {
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	for (int i = 0; i < sideLength; ++i) {
		matrix[i] = i * gapScore;
	}
	for (int i = 1; i < sideLength; ++i) {
		matrix[i * sideLength] = i * gapScore;
		for (int j = 1; j < sideLength; ++j) {
			matrix[i * sideLength + j] = std::max(
					matrix[(i - 1) * sideLength + (j - 1)] + 2 * ((interval - abs(a[i - 1] - b[j - 1])) / interval) - 1,
					std::max(
							matrix[(i - 1) * sideLength + j] + gapScore,
							matrix[i * sideLength + (j - 1)] + gapScore
					)
			);
		}
	}
	return matrix[matrixSize - 1];
}

double batchAlignmentScore(const int* A, const int* B, int rows, int columns, int minVal, int maxVal, int gapScore) {
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	std::vector<double> matrix(matrixSize, 0);
	double interval = maxVal - minVal;
	double totalScore = 0;
	for (int i = 0; i < rows; ++i)
		totalScore += alignmentScore(matrix, A + i * columns, B + i * columns, columns, interval, gapScore);
	return totalScore;
}

const int SIMPLE_MAX_PIXEL_DISTANCE = 255 * 3;
const int V = SIMPLE_MAX_PIXEL_DISTANCE;
const double B = V / 2.0;
const double V_PLUS_B = V + B;

inline double moderate(double x) {
	return V_PLUS_B * x / (x  + B);
}

inline double pixelSimilarity(const Sequence* p1, int indexP1, const Sequence* p2, int indexP2) {
	return (SIMPLE_MAX_PIXEL_DISTANCE - moderate(
			std::abs(p1->r[indexP1] - p2->r[indexP2])
			+ std::abs(p1->g[indexP1] - p2->g[indexP2])
			+ std::abs(p1->b[indexP1] - p2->b[indexP2])
			)) / SIMPLE_MAX_PIXEL_DISTANCE;
}

#define PIXEL_SIMILARITY(p1, x, y, p2, localX, localY, width) pixelSimilarity(p1, (x) + (y) * (width), p2, (localX) + (localY) * (width))

template <typename T>
T getMax(T t1, T t2, T t3, T t4, T t5 = -1, T t6 = -1, T t7 = -1, T t8 = -1, T t9 = -1) {
	T val = t1;
	if (val < t2) val = t2;
	if (val < t3) val = t3;
	if (val < t4) val = t4;
	if (val < t5) val = t5;
	if (val < t6) val = t6;
	if (val < t7) val = t7;
	if (val < t8) val = t8;
	if (val < t9) val = t9;
	return val;
}

inline double compare(const Sequence* p1, const Sequence* p2, int width, int height, int size) {
	// x, y:
	// 0, 0
	double totalScore = getMax(
			PIXEL_SIMILARITY(p1, 0 ,0, p2, 0, 0, width),
			PIXEL_SIMILARITY(p1, 0 ,0, p2, 1, 0, width),
			PIXEL_SIMILARITY(p1, 0 ,0, p2, 0, 1, width),
			PIXEL_SIMILARITY(p1, 0 ,0, p2, 1, 1, width));
	// width - 1, 0
	totalScore += getMax(
			PIXEL_SIMILARITY(p1, width - 1, 0, p2, width - 2, 0, width),
			PIXEL_SIMILARITY(p1, width - 1, 0, p2, width - 1, 0, width),
			PIXEL_SIMILARITY(p1, width - 1, 0, p2, width - 2, 1, width),
			PIXEL_SIMILARITY(p1, width - 1, 0, p2, width - 1, 1, width));
	// 0, height - 1
	totalScore += getMax(
			PIXEL_SIMILARITY(p1, 0, height - 1, p2, 0, height - 1, width),
			PIXEL_SIMILARITY(p1, 0, height - 1, p2, 1, height - 1, width),
			PIXEL_SIMILARITY(p1, 0, height - 1, p2, 0, height - 2, width),
			PIXEL_SIMILARITY(p1, 0, height - 1, p2, 1, height - 2, width));
	// width - 1, height - 1
	totalScore += getMax(
			PIXEL_SIMILARITY(p1, width - 1, height - 1, p2, width - 2, height - 1, width),
			PIXEL_SIMILARITY(p1, width - 1, height - 1, p2, width - 1, height - 1, width),
			PIXEL_SIMILARITY(p1, width - 1, height - 1, p2, width - 2, height - 2, width),
			PIXEL_SIMILARITY(p1, width - 1, height - 1, p2, width - 1, height - 2, width));
	for (int x = 1; x <= width - 2; ++x) {
		// x, 0
		totalScore += getMax(
				PIXEL_SIMILARITY(p1, x, 0, p2, x - 1, 0, width),
				PIXEL_SIMILARITY(p1, x, 0, p2, x, 0, width),
				PIXEL_SIMILARITY(p1, x, 0, p2, x + 1, 0, width),
				PIXEL_SIMILARITY(p1, x, 0, p2, x - 1, 1, width),
				PIXEL_SIMILARITY(p1, x, 0, p2, x, 1, width),
				PIXEL_SIMILARITY(p1, x, 0, p2, x + 1, 1, width));
		// x, height - 1
		totalScore += getMax(
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x - 1, height - 1, width),
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x, height - 1, width),
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x + 1, height - 1, width),
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x - 1, height - 2, width),
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x, height - 2, width),
				PIXEL_SIMILARITY(p1, x, height - 1, p2, x + 1, height - 2, width));
	}
	for (int y = 1; y <= height - 2; ++y) {
		// 0, y
		totalScore += getMax(
				PIXEL_SIMILARITY(p1, 0, y, p2, 0, y - 1, width),
				PIXEL_SIMILARITY(p1, 0, y, p2, 1, y - 1, width),
				PIXEL_SIMILARITY(p1, 0, y, p2, 0, y, width),
				PIXEL_SIMILARITY(p1, 0, y, p2, 1, y, width),
				PIXEL_SIMILARITY(p1, 0, y, p2, 0, y + 1, width),
				PIXEL_SIMILARITY(p1, 0, y, p2, 1, y + 1, width));
		// width - 1, y
		totalScore += getMax(
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 2, y - 1, width),
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 1, y - 1, width),
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 2, y, width),
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 1, y, width),
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 2, y + 1, width),
				PIXEL_SIMILARITY(p1, width - 1, y, p2, width - 1, y + 1, width));
	}
	// x in [1; width - 2], y in [1; height - 2]
	int remainingSize = (width - 2) * (height - 2);
	for (int index = 0; index < remainingSize; ++index) {
		int x = index % (width - 2) + 1;
		int y = index / (width - 2) + 1;
		totalScore += getMax(
				PIXEL_SIMILARITY(p1, x, y, p2, x - 1, y - 1, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x, y - 1, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x + 1, y - 1, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x - 1, y, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x, y, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x + 1, y, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x - 1, y + 1, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x, y + 1, width),
				PIXEL_SIMILARITY(p1, x, y, p2, x + 1, y + 1, width));
	}
	return totalScore / size;
}

void sub(Sequence** sequences, int width, int height, int size, double similarityLimit, int i, int jFrom, int jTo) {
	for (int j = jFrom; j < jTo; ++j) {
		double score = compare(sequences[i], sequences[j], width, height, size);
		if (score >= similarityLimit) {
			sequences[j]->classification = sequences[i]->classification;
			sequences[j]->score = score;
		}
	}
}

void classifySimilarities(Sequence** sequences, int nbSequences, int width, int height, double similarityLimit, int v) {
	std::cout << "STARTING" << std::endl;
	int size = width * height;
	int nbThreads = 4;
	for (int i = 0; i < nbSequences - 1; ++i) {
		int a = i + 1;
		int l = (nbSequences - i - 1) / nbThreads;
		std::vector<std::thread> processes(nbThreads);
		for (int t = 0; t < nbThreads - 1; ++t) {
			processes[t] = std::thread(sub, sequences, width, height, size, similarityLimit, i, a + t * l, a + (t + 1) * l);
		}
		processes[nbThreads - 1] = std::thread(
				sub, sequences, width, height, size, similarityLimit, i, a + (nbThreads - 1) * l, nbSequences);
		for (auto& process: processes)
			process.join();
		if ((i + 1) % 100 == 0) {
			std::cout
				<< "(*) Image " << i + 1 << " / " << nbSequences
				<< " (" << l << " per thread on " << nbThreads << " threads)" << std::endl;
		}
	}
}