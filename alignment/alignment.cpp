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
	for (int j = 1; j < sideLength; ++j) {
		matrix[j * sideLength] = j * gapScore;
	}
	for (int i = 1; i < sideLength; ++i) {
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
	double totalScore = 0;
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	std::vector<double> matrix(matrixSize, 0);
	double interval = maxVal - minVal;
	for (int i = 0; i < rows; ++i)
		totalScore += alignmentScore(matrix, A + i * columns, B + i * columns, columns, interval, gapScore);
	return totalScore;
}

bool classifiedSequenceComparator(Sequence* a, Sequence* b) {
	return a->classification < b->classification || (a->classification == b->classification && a->score < b->score);
}

struct Aligner {
	int rows;
	int columns;
	int minVal;
	int maxVal;
	int gapScore;
	int minAlignmentScore;
	int maxAlignmentScore;

	Aligner(int theRows, int theColumns, int theMinVal, int theMaxVal, int theGapScore) :
			rows(theRows), columns(theColumns), minVal(theMinVal), maxVal(theMaxVal), gapScore(theGapScore) {
		minAlignmentScore = std::min(-1, gapScore) * rows * columns;
		maxAlignmentScore = std::max(1, gapScore) * rows * columns;
	}

	double align(Sequence* a, Sequence* b) {
		double scoreR = batchAlignmentScore(a->r, b->r, rows, columns, minVal, maxVal, gapScore);
		double scoreG = batchAlignmentScore(a->g, b->g, rows, columns, minVal, maxVal, gapScore);
		double scoreB = batchAlignmentScore(a->b, b->b, rows, columns, minVal, maxVal, gapScore);
		return (scoreR + scoreG + scoreB - 3 * minAlignmentScore) / (3 * (maxAlignmentScore - minAlignmentScore));
	}
};

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