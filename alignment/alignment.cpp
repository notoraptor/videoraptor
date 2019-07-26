//
// Created by notoraptor on 27/06/2019.
//

#include <algorithm>
#include <vector>
#include <iostream>
#include <cmath>
#include <omp.h>
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

inline double pixelDistance(const Sequence* p1, int indexP1, const Sequence* p2, int indexP2) {
	return moderate(
			std::abs(p1->r[indexP1] - p2->r[indexP2])
			+ std::abs(p1->g[indexP1] - p2->g[indexP2])
			+ std::abs(p1->b[indexP1] - p2->b[indexP2]));
}

#define PIXEL_DISTANCE(p1, x, y, p2, localX, localY, width) pixelDistance(p1, (x) + (y) * (width), p2, (localX) + (localY) * (width))

template <typename T>
T getMin(T t1, T t2, T t3, T t4, T t5 = V, T t6 = V, T t7 = V, T t8 = V, T t9 = V) {
	T val = t1;
	if (val > t2) val = t2;
	if (val > t3) val = t3;
	if (val > t4) val = t4;
	if (val > t5) val = t5;
	if (val > t6) val = t6;
	if (val > t7) val = t7;
	if (val > t8) val = t8;
	if (val > t9) val = t9;
	return val;
}

inline double compareFaster(const Sequence* p1, const Sequence* p2, int width, int height, int maximumSimilarityScore) {
	// x, y:
	// 0, 0
	double totalDistance = getMin(
			PIXEL_DISTANCE(p1, 0 ,0, p2, 0, 0, width),
			PIXEL_DISTANCE(p1, 0 ,0, p2, 1, 0, width),
			PIXEL_DISTANCE(p1, 0 ,0, p2, 0, 1, width),
			PIXEL_DISTANCE(p1, 0 ,0, p2, 1, 1, width));
	// width - 1, 0
	totalDistance += getMin(
			PIXEL_DISTANCE(p1, width - 1, 0, p2, width - 2, 0, width),
			PIXEL_DISTANCE(p1, width - 1, 0, p2, width - 1, 0, width),
			PIXEL_DISTANCE(p1, width - 1, 0, p2, width - 2, 1, width),
			PIXEL_DISTANCE(p1, width - 1, 0, p2, width - 1, 1, width));
	// 0, height - 1
	totalDistance += getMin(
			PIXEL_DISTANCE(p1, 0, height - 1, p2, 0, height - 1, width),
			PIXEL_DISTANCE(p1, 0, height - 1, p2, 1, height - 1, width),
			PIXEL_DISTANCE(p1, 0, height - 1, p2, 0, height - 2, width),
			PIXEL_DISTANCE(p1, 0, height - 1, p2, 1, height - 2, width));
	// width - 1, height - 1
	totalDistance += getMin(
			PIXEL_DISTANCE(p1, width - 1, height - 1, p2, width - 2, height - 1, width),
			PIXEL_DISTANCE(p1, width - 1, height - 1, p2, width - 1, height - 1, width),
			PIXEL_DISTANCE(p1, width - 1, height - 1, p2, width - 2, height - 2, width),
			PIXEL_DISTANCE(p1, width - 1, height - 1, p2, width - 1, height - 2, width));
	// x, 0
	for (int x = 1; x <= width - 2; ++x) {
		totalDistance += getMin(
				PIXEL_DISTANCE(p1, x, 0, p2, x - 1, 0, width),
				PIXEL_DISTANCE(p1, x, 0, p2, x, 0, width),
				PIXEL_DISTANCE(p1, x, 0, p2, x + 1, 0, width),
				PIXEL_DISTANCE(p1, x, 0, p2, x - 1, 1, width),
				PIXEL_DISTANCE(p1, x, 0, p2, x, 1, width),
				PIXEL_DISTANCE(p1, x, 0, p2, x + 1, 1, width));
	}
	// x, height - 1
	for (int x = 1; x <= width - 2; ++x) {
		totalDistance += getMin(
				PIXEL_DISTANCE(p1, x, height - 1, p2, x - 1, height - 1, width),
				PIXEL_DISTANCE(p1, x, height - 1, p2, x, height - 1, width),
				PIXEL_DISTANCE(p1, x, height - 1, p2, x + 1, height - 1, width),
				PIXEL_DISTANCE(p1, x, height - 1, p2, x - 1, height - 2, width),
				PIXEL_DISTANCE(p1, x, height - 1, p2, x, height - 2, width),
				PIXEL_DISTANCE(p1, x, height - 1, p2, x + 1, height - 2, width));
	}
	for (int y = 1; y <= height - 2; ++y) {
		// 0, y
		totalDistance += getMin(
				PIXEL_DISTANCE(p1, 0, y, p2, 0, y - 1, width),
				PIXEL_DISTANCE(p1, 0, y, p2, 1, y - 1, width),
				PIXEL_DISTANCE(p1, 0, y, p2, 0, y, width),
				PIXEL_DISTANCE(p1, 0, y, p2, 1, y, width),
				PIXEL_DISTANCE(p1, 0, y, p2, 0, y + 1, width),
				PIXEL_DISTANCE(p1, 0, y, p2, 1, y + 1, width));
		// width - 1, y
		totalDistance += getMin(
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 2, y - 1, width),
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 1, y - 1, width),
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 2, y, width),
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 1, y, width),
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 2, y + 1, width),
				PIXEL_DISTANCE(p1, width - 1, y, p2, width - 1, y + 1, width));
	}
	// x in [1; width - 2], y in [1; height - 2]
	int remainingSize = (width - 2) * (height - 2);
	for (int index = 0; index < remainingSize; ++index) {
		int x = index % (width - 2) + 1;
		int y = index / (width - 2) + 1;
		totalDistance += getMin(
				PIXEL_DISTANCE(p1, x, y, p2, x - 1, y - 1, width),
				PIXEL_DISTANCE(p1, x, y, p2, x, y - 1, width),
				PIXEL_DISTANCE(p1, x, y, p2, x + 1, y - 1, width),
				PIXEL_DISTANCE(p1, x, y, p2, x - 1, y, width),
				PIXEL_DISTANCE(p1, x, y, p2, x, y, width),
				PIXEL_DISTANCE(p1, x, y, p2, x + 1, y, width),
				PIXEL_DISTANCE(p1, x, y, p2, x - 1, y + 1, width),
				PIXEL_DISTANCE(p1, x, y, p2, x, y + 1, width),
				PIXEL_DISTANCE(p1, x, y, p2, x + 1, y + 1, width));
	}
	return (maximumSimilarityScore - totalDistance) / maximumSimilarityScore;
}

void sub(Sequence** sequences, int width, int height, int maximumSimilarityScore, double similarityLimit, int i, int jFrom, int jTo) {
	#pragma omp parallel for default(none) shared(sequences, i, jFrom, jTo, width, height, maximumSimilarityScore, similarityLimit)
	for (int j = jFrom; j < jTo; ++j) {
		double score = compareFaster(sequences[i], sequences[j], width, height, maximumSimilarityScore);
		if (score >= similarityLimit) {
			sequences[j]->classification = sequences[i]->classification;
			sequences[j]->score = score;
		}
	}
}

void classifySimilarities(Sequence** sequences, int nbSequences, int width, int height, double similarityLimit, int v) {
	std::cout << "STARTING" << std::endl;
	int maximumSimilarityScore = SIMPLE_MAX_PIXEL_DISTANCE * width * height;
	for (int i = 0; i < nbSequences - 1; ++i) {
		#pragma omp parallel for default(none) shared(sequences, i, nbSequences, width, height, similarityLimit, maximumSimilarityScore)
		for (int j = i + 1; j < nbSequences; ++j) {
			double score = compareFaster(sequences[i], sequences[j], width, height, maximumSimilarityScore);
			if (score >= similarityLimit) {
				sequences[j]->classification = sequences[i]->classification;
				sequences[j]->score = score;
			}
		}
		if ((i + 1) % 100 == 0) {
			std::cout << "(*) Image " << i + 1 << " / " << nbSequences << std::endl;
		}
	}
}
