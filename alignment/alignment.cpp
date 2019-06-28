//
// Created by notoraptor on 27/06/2019.
//

#include <algorithm>
#include <vector>
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
					matrix[(i - 1) * sideLength + (j - 1)] + 2 * ((interval - abs(a[i - 1] - b[j - 1]))/interval) - 1,
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

void Sequence_init(Sequence* sequence) {
	sequence->classification = -1;
	sequence->score = 0;
}

bool classifiedSequenceComparator(Sequence* a, Sequence* b) {
	return a->classification < b->classification || a->score < b->score;
}

double alignSequences(Sequence* a, Sequence* b, int rows, int columns, int minVal, int maxVal, int gapScore) {
	double scoreRed = batchAlignmentScore(a->red, b->red, rows, columns, minVal, maxVal, gapScore);
	double scoreGreen = batchAlignmentScore(a->green, b->green, rows, columns, minVal, maxVal, gapScore);
	double scoreBlue = batchAlignmentScore(a->blue, b->blue, rows, columns, minVal, maxVal, gapScore);
	int minAlignmentScore = std::min(-1, gapScore) * rows * columns;
	int maxAlignmentScore = std::max(1, gapScore) * rows * columns;
	return (scoreRed + scoreGreen + scoreBlue - 3 * minAlignmentScore) / (3 * (maxAlignmentScore - minAlignmentScore));
}

int classifySimilarities(
		Sequence** rawSequences, int nbSequences, double similarityLimit, double differenceLimit,
		int rows, int columns, int minVal, int maxVal, int gapScore) {
	std::vector<Sequence*> sequences(rawSequences, rawSequences + nbSequences);
	if (sequences.size() != (size_t)nbSequences)
		return -1;
	// Initialize first sequence.
	sequences[0]->classification = 0;
	sequences[0]->score = 1;
	int nbFoundSimilarSequences = 1;
	// Compute score and match (0) / mismatch (1) classification for next sequences vs first sequence.
	for (int i = 1; i < nbSequences; ++i) {
		sequences[i]->score = alignSequences(sequences[0], sequences[i], rows, columns, minVal, maxVal, gapScore);
		if (sequences[i]->score >= similarityLimit) {
			sequences[i]->classification = 0;
			++nbFoundSimilarSequences;
		} else {
			sequences[i]->classification = 1;
		}
	}
	if (nbFoundSimilarSequences != 1) {
		// We found sequences similar to the first one.
		// Sort sequences by classification (put similar sequences at top) then by score.
		std::sort(sequences.begin(), sequences.end(), classifiedSequenceComparator);
	}
	int nextClass = 1;
	int start = nbFoundSimilarSequences;
	int cursor = start + 1;
	while (cursor < nbSequences) {
		double startScore = sequences[start]->score;
		double currScore = sequences[cursor]->score;
		double distance = startScore - currScore;
		if (distance < 0)
			distance = -distance;
		if (distance > differenceLimit) {
			// Potential similar group in [start; cursor - 1]
			for (int i = start; i < cursor; ++i) {
				sequences[i]->classification = nextClass;
			}
			++nextClass;
			if (cursor == nbSequences - 1) {
				sequences[cursor]->classification = nextClass;
				++nextClass;
			} else {
				start = cursor;
			}
		} else if (cursor == nbSequences - 1) {
			for (int i = start; i < nbSequences; ++i) {
				sequences[i]->classification = nextClass;
			}
			++nextClass;
		}
		++cursor;
	}
	return nbFoundSimilarSequences;
}