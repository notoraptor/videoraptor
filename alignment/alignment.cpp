//
// Created by notoraptor on 27/06/2019.
//

#include <algorithm>
#include <vector>
#include <iostream>
#include <cstring>
#include <thread>
#include <cmath>
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

struct PixelClass {
	uint8_t red, green, blue;
	PixelClass(uint8_t r, uint8_t g, uint8_t b): red(0), green(0), blue(0) {
		if (r >  g && g == b) {red = 1; green = 0; blue = 0;}
		else if (g >  r && r == b) {red = 0; green = 1; blue = 0;}
		else if (b >  r && r == g) {red = 0; green = 0; blue = 1;}
		else if (r == g && g >  b) {red = 1; green = 1; blue = 0;}
		else if (r == b && b >  g) {red = 1; green = 0; blue = 1;}
		else if (g == b && b >  r) {red = 0; green = 1; blue = 1;}
		else if (r == g && g == b) {red = 0; green = 0; blue = 0;}
		else if (r >  g && g >  b) {red = 2; green = 1; blue = 0;}
		else if (r >  b && b >  g) {red = 2; green = 0; blue = 1;}
		else if (g >  r && r >  b) {red = 1; green = 2; blue = 0;}
		else if (g >  b && b >  r) {red = 0; green = 2; blue = 1;}
		else if (b >  r && r >  g) {red = 1; green = 0; blue = 2;}
		else if (b >  g && g >  r) {red = 0; green = 1; blue = 2;}
	}

	int distance(const PixelClass& other) {
		return std::abs(red - other.red) + std::abs(green - other.green) + std::abs(blue - other.blue);
	}

};

const double MAX_PIXEL_CLASS_DISTANCE = 4;
const double MAX_PIXEL_DISTANCE = 255 * sqrt(3);

double pixelDistance(int r1, int g1, int b1, int r2, int g2, int b2) {
	return sqrt((r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2));
}

double pixelSimilarity(int r1, int g1, int b1, int r2, int g2, int b2) {
	double dp = pixelDistance(r1, g1, b1, r2, g2, b2);
	double normalized_dc = PixelClass(r1, g1, b1).distance(PixelClass(r2, g2, b2)) / MAX_PIXEL_CLASS_DISTANCE;
	return (MAX_PIXEL_DISTANCE - dp * (1 + 3 * normalized_dc) / 4) / MAX_PIXEL_DISTANCE;
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

	double computeAlignmentScore(const Sequence* a, const Sequence* b, int rowIndex) {
		int* rowAR = a->r + rowIndex;
		int* rowAG = a->g + rowIndex;
		int* rowAB = a->b + rowIndex;
		int* rowBR = b->r + rowIndex;
		int* rowBG = b->g + rowIndex;
		int* rowBB = b->b + rowIndex;
		for (int i = 0; i < sideLength; ++i) {
			matrix[i] = i * gapScore;
		}
		for (int i = 1; i < sideLength; ++i) {
			matrix[i * sideLength] = i * gapScore;
			for (int j = 1; j < sideLength; ++j) {
				matrix[i * sideLength + j] = std::max(
						matrix[(i - 1) * sideLength + (j - 1)] + 2 * pixelSimilarity(
								rowAR[i - 1], rowAG[i - 1], rowAB[i - 1],
								rowBR[j - 1], rowBG[j - 1], rowBB[j - 1]
								) - 1,
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

	double computeBatchAlignmentScore(const Sequence* a, const Sequence* b) {
		double totalScore = 0;
		for (int i = 0; i < rows; ++i)
			totalScore += computeAlignmentScore(a, b, i * columns);
		return totalScore;
	}

	double align(const Sequence* a, const Sequence* b) {
		double scoreR = computeBatchAlignmentScore(a->r, b->r);
		double scoreG = computeBatchAlignmentScore(a->g, b->g);
		double scoreB = computeBatchAlignmentScore(a->b, b->b);
		return (scoreR + scoreG + scoreB - 3 * minAlignmentScore) / (3 * alignmentScoreInterval);
	}

	double align2(const Sequence* a, const Sequence* b) {
		return (computeBatchAlignmentScore(a, b) - minAlignmentScore) / alignmentScoreInterval;
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

inline uint64_t arrayDistance(const int* a, const int* b, int len) {
	uint64_t total = 0;
	for (int i = 0; i < len; ++i)
		total += std::abs(a[i] - b[i]);
	return total;
}

void sub(Sequence** sequences, int n, double similarityLimit, int v, int i, int jFrom, int jTo) {
	int side = int(sqrt(n));
	Aligner aligner(side, side, 0, v, -1);
	double alignmentLimit = similarityLimit + ((1 - similarityLimit) / 2);
	alignmentLimit = 0.94;
	alignmentLimit = 0.91;
	for (int j = jFrom; j < jTo; ++j) {
		if (sequences[j]->classification == -1)
			sequences[j]->classification = j;
		double score = (
				uint64_t(3) * n * v
				- arrayDistance(sequences[i]->r, sequences[j]->r, n)
				- arrayDistance(sequences[i]->g, sequences[j]->g, n)
				- arrayDistance(sequences[i]->b, sequences[j]->b, n)
				)/ double(3 * n * v);
		if (score >= similarityLimit) {
			score = aligner.align2(sequences[i], sequences[j]);
			// std::cout << "[" << i << " " << j << "] " << score << std::endl;
			if (score >= alignmentLimit) {
				sequences[j]->classification = sequences[i]->classification;
				sequences[j]->score = score;
			}
		}
	}
}

void classifySimilarities2(Sequence** sequences, int nbSequences, int n, double similarityLimit, int v) {
	int side = int(sqrt(n));
	if (side * side != n) {
		std::cerr << "Error: " << side << "^2 != " << n << std::endl;
		return;
	}
	int nbThreads = 4;
	for (int i = 0; i < nbSequences - 1; ++i) {
		if (sequences[i]->classification == -1)
			sequences[i]->classification = i;
		int a = i + 1;
		int l = (nbSequences - i - 1) / nbThreads;
		std::vector<std::thread> processes(nbThreads);
		for (int t = 0; t < nbThreads - 1; ++t) {
			processes[t] = std::thread(sub, sequences, n, similarityLimit, v, i, a + t * l, a + (t + 1) * l);
		}
		processes[nbThreads - 1] = std::thread(sub, sequences, n, similarityLimit, v, i, a + (nbThreads - 1) * l, nbSequences);
		for (auto& process: processes)
			process.join();
		if ((i + 1) % 1000 == 0) {
			std::cout
				<< "(*) Image " << i + 1 << " / " << nbSequences
				<< " (" << l << " per thread on " << nbThreads << " threads)" << std::endl;
		}
	}
}