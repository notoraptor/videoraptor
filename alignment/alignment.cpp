//
// Created by notoraptor on 27/06/2019.
//

#include <algorithm>
#include <vector>
#include <iostream>
#include <cstring>
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

struct Aligner {
	int rows;
	int columns;
	int minVal;
	int maxVal;
	int gapScore;

	int minAlignmentScore;
	int maxAlignmentScore;
	int sideLength;
	double interval;
	int alignmentScoreInterval;
	int matrixSize;
	std::vector<double> matrix;

	Aligner(int theRows, int theColumns, int theMinVal, int theMaxVal, int theGapScore) :
			rows(theRows), columns(theColumns), minVal(theMinVal), maxVal(theMaxVal), gapScore(theGapScore),
			minAlignmentScore(std::min(-1, theGapScore) * theRows * theColumns),
			maxAlignmentScore(std::max(+1, theGapScore) * theRows * theColumns),
			sideLength(theColumns + 1),
			interval(theMaxVal - theMinVal),
			matrix() {
		alignmentScoreInterval = maxAlignmentScore - minAlignmentScore;
		matrixSize = sideLength * sideLength;
		matrix.assign(matrixSize, 0);
	}

	double computeAlignmentScore(const int* a, const int* b) {
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

	double computeBatchAlignmentScore(const int* A, const int* B) {
		double totalScore = 0;
		for (int i = 0; i < rows; ++i)
			totalScore += computeAlignmentScore(A + i * columns, B + i * columns);
		return totalScore;
	}

	double align(Sequence* a, Sequence* b) {
		double scoreR = computeBatchAlignmentScore(a->r, b->r);
		double scoreG = computeBatchAlignmentScore(a->g, b->g);
		double scoreB = computeBatchAlignmentScore(a->b, b->b);
		return (scoreR + scoreG + scoreB - 3 * minAlignmentScore) / (3 * alignmentScoreInterval);
	}
};

bool classifiedSequenceComparator(Sequence* a, Sequence* b) {
	return a->classification < b->classification || (a->classification == b->classification && a->score < b->score);
}

int classifySimilarities(
		Sequence** rawSequences, int nbSequences, double similarityLimit, double differenceLimit,
		int rows, int columns, int minVal, int maxVal, int gapScore) {
	Aligner aligner(rows, columns, minVal, maxVal, gapScore);
	std::vector<Sequence*> sequences(nbSequences, nullptr);
	memcpy(sequences.data(), rawSequences, nbSequences * sizeof(Sequence*));
	// Initialize first sequence.
	sequences[0]->classification = 0;
	sequences[0]->score = 1;
	int nbFoundSimilarSequences = 1;
	// Compute score and match (0) / mismatch (1) classification for next sequences vs first sequence.
	for (int i = 1; i < nbSequences; ++i) {
		sequences[i]->score = aligner.align(sequences[0], sequences[i]);
		if (sequences[i]->score >= similarityLimit) {
			sequences[i]->classification = 0;
			++nbFoundSimilarSequences;
		} else {
			sequences[i]->classification = 1;
		}
	}
	// Sort sequences by classification (put similar sequences at top) then by score.
	std::sort(sequences.begin(), sequences.end(), classifiedSequenceComparator);
	int nextClass = 1;
	int start = nbFoundSimilarSequences;
	int cursor = start + 1;
	// Find first group of potential similar sequences.
	// If found, group is already set with class 1, so we will just update next class.
	while (cursor < nbSequences) {
		double distance = sequences[start]->score - sequences[cursor]->score;
		if (distance < -differenceLimit || distance > differenceLimit) {
			++nextClass;
			start = cursor;
			break;
		}
		++cursor;
	}
	// Find next groups of potential similar sequences.
	while (cursor < nbSequences) {
		double distance = sequences[start]->score - sequences[cursor]->score;
		if (distance < -differenceLimit || distance > differenceLimit) {
			// Potential similar group in [start; cursor - 1]
			for (int i = start; i < cursor; ++i) {
				sequences[i]->classification = nextClass;
			}
			++nextClass;
			start = cursor;
		}
		++cursor;
	}
	for (int i = start; i < nbSequences; ++i) {
		sequences[i]->classification = nextClass;
	}
	return nbFoundSimilarSequences;
}