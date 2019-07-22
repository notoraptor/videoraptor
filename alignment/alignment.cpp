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
const int MAX_PIXEL_DISTANCE_2 = 255 * 3;

inline double pixelDistance(double r1, int g1, int b1, int r2, int g2, int b2) {
	return sqrt((r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2));
}

inline double positionDistance(double x1, int y1, int x2, int y2) {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

inline double superModerate(double x, double V, double b) {
	return (V + b) * x / (x + b);
}

inline int moderate(int x, int V, int b) {
	return (V + b) * x / (x + b);
}

inline int pixelDistance2(int r1, int g1, int b1, int r2, int g2, int b2) {
	return std::abs(r1 - r2) + std::abs(g1 - g2) + std::abs(b1 - b2);
}

inline int positionDistance2(int x1, int y1, int x2, int y2) {
	return std::abs(x1 - x2) + std::abs(y1 - y2);
}

const double V = MAX_PIXEL_DISTANCE;
const double H = V / 2;
const double P = V / 4;

struct AlternateModerator {
	double V;
	double h;
	double p;
	double m;
	double q;
	AlternateModerator(double theV, double theH, double theP): V(theV), h(theH), p(theP) {
		m = V * (V * h + V * p - h * h + p * p) / (2 * V * h + V * p - 2 * h * h);
		q = m * h / (h + p);
		if (!(V > 0 && h > 0 && p > 0 && m > 0)) {
			std::cerr << "[WARNING] V, h, p or m is <= 0. V = " << V << ", h = " << h << ", p = " << p << ", m = " << m << std::endl;
		}
	}

	void debug() {
		std::cout << "[INFO] " << "f(x)=" << m << "(x-" << h << ")/(|x-" << h << "|+" << p << ")+" << q << std::endl;
	}

	double moderate(double x) const {
		return m * (x - h) / (std::abs(x - h) + p) + q;
	}

	double pixelSimilarity(int r1, int g1, int b1, int r2, int g2, int b2) const {
		double dp = pixelDistance(r1, g1, b1, r2, g2, b2);
		double normalizedPixelClassDistance = PixelClass(r1, g1, b1).distance(PixelClass(r2, g2, b2)) / MAX_PIXEL_CLASS_DISTANCE;
		return (MAX_PIXEL_DISTANCE - this->moderate(dp * (1 + normalizedPixelClassDistance) / 2)) / MAX_PIXEL_DISTANCE;
	}
};

inline double pixelSimilarity(int r1, int g1, int b1, int r2, int g2, int b2) {
	 double dp = pixelDistance(r1, g1, b1, r2, g2, b2);
	 double normalizedPixelClassDistance = PixelClass(r1, g1, b1).distance(PixelClass(r2, g2, b2)) / MAX_PIXEL_CLASS_DISTANCE;
	 return (MAX_PIXEL_DISTANCE - dp * (1 + normalizedPixelClassDistance) / 2) / MAX_PIXEL_DISTANCE;
	 // return (MAX_PIXEL_DISTANCE - pixelDistance(r1, g1, b1, r2, g2, b2)) / MAX_PIXEL_DISTANCE;
	// return (double(MAX_PIXEL_DISTANCE_2) - pixelDistance2(r1, g1, b1, r2, g2, b2)) / MAX_PIXEL_DISTANCE_2;
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
	AlternateModerator alternateModerator;

	Aligner(int theRows, int theColumns, int theMinVal, int theMaxVal, int theGapScore) :
			rows(theRows), columns(theColumns), minVal(theMinVal), maxVal(theMaxVal), gapScore(theGapScore),
			minAlignmentScore(std::max(2 * theGapScore, std::min(-1, theGapScore)) * theRows * theColumns),
			maxAlignmentScore(std::max(+1, theGapScore) * theRows * theColumns),
			sideLength(theColumns + 1),
			interval(theMaxVal - theMinVal),
			matrix(),
			alternateModerator(V, H, P) {
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

	double computeAlignmentScore(const Sequence* a, const Sequence* b, int start, int step) {
		for (int i = 0; i < sideLength; ++i) {
			matrix[i] = i * gapScore;
		}
		for (int i = 1; i < sideLength; ++i) {
			matrix[i * sideLength] = i * gapScore;
			for (int j = 1; j < sideLength; ++j) {
				matrix[i * sideLength + j] = std::max(
						matrix[(i - 1) * sideLength + (j - 1)] + 2 * alternateModerator.pixelSimilarity(
								*(a->r + start + step * (i - 1)), *(a->g + start + step * (i - 1)), *(a->b + start + step * (i - 1)),
								*(b->r + start + step * (j - 1)), *(b->g + start + step * (j - 1)), *(b->b + start + step * (j - 1))
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

	double computeBatchAlignmentScore(const Sequence* a, const Sequence* b, int nSteps, int leadStep, int cellStep) {
		double totalScore = 0;
		for (int i = 0; i < nSteps; ++i)
			totalScore += computeAlignmentScore(a, b, i * leadStep, cellStep);
		return totalScore;
	}

	double align(const Sequence* a, const Sequence* b) {
		double scoreR = computeBatchAlignmentScore(a->r, b->r);
		double scoreG = computeBatchAlignmentScore(a->g, b->g);
		double scoreB = computeBatchAlignmentScore(a->b, b->b);
		return (scoreR + scoreG + scoreB - 3 * minAlignmentScore) / (3 * alignmentScoreInterval);
	}

	double align2(const Sequence* a, const Sequence* b) {
		///*
		double scoreRows = (computeBatchAlignmentScore(a, b, rows, columns, 1) - minAlignmentScore) / alignmentScoreInterval;
		double scoreCols = (computeBatchAlignmentScore(a, b, columns, 1, rows) - minAlignmentScore) / alignmentScoreInterval;
		return std::max(scoreRows, scoreCols);
		//*/
		// return (computeBatchAlignmentScore(a, b, rows, columns, 1) - minAlignmentScore) / alignmentScoreInterval;
	}
};

inline uint64_t arrayDistance(const int* a, const int* b, int len) {
	uint64_t total = 0;
	for (int i = 0; i < len; ++i)
		total += std::abs(a[i] - b[i]);
	return total;
}

class RawComparator {
	const Sequence* referenceSequence;
	int length;
	int width;
	int height;
	int maxTotalDistance;
	int maxPositionDistance;

public:
	RawComparator(const Sequence* baseSequence, int sequenceWidth, int sequenceHeight):
			referenceSequence(baseSequence), length(sequenceWidth * sequenceHeight),
			width(sequenceWidth), height(sequenceHeight) {
		maxPositionDistance = positionDistance2(0, 0, width - 1, height - 1);
		maxTotalDistance = length * MAX_PIXEL_DISTANCE_2 * maxPositionDistance;
	}
	double similarity(const Sequence* currentSequence) {
		int totalDistance = 0;
		for (int i = 0; i < length; ++i) {
			int localPixelDistance = pixelDistance2(
					referenceSequence->r[referenceSequence->i[i]],
					referenceSequence->g[referenceSequence->i[i]],
					referenceSequence->b[referenceSequence->i[i]],
					currentSequence->r[currentSequence->i[i]],
					currentSequence->g[currentSequence->i[i]],
					currentSequence->b[currentSequence->i[i]]
			);
			int localPositionDistance = positionDistance2(
					referenceSequence->i[i] % width,
					referenceSequence->i[i] / width,
					currentSequence->i[i] % width,
					currentSequence->i[i] / width
			);
			totalDistance += localPixelDistance * moderate(localPositionDistance, maxPositionDistance, 1);
		}
		return double(maxTotalDistance - totalDistance) / maxTotalDistance;
	}
};


inline int nDigits(int value) {
	int n = 0;
	while (value) {
		value /= 10;
		++n;
	}
	return n;
}

inline int tenPower(int value) {
	int n = 1;
	while (value) {
		n *= 10;
		--value;
	}
	return n;
}

inline double squareRoot(int value) {
	// The Babylonian method for finding square roots by hand (2019/07/22)
	// https://blogs.sas.com/content/iml/2016/05/16/babylonian-square-roots.html
	double x = tenPower(nDigits(value) / 2);
	while (std::abs(value - x * x) >= 0.05) {
		x = (x + value/x)/2;
	}
	return x;
}

inline double pixelSimilarity(const Sequence* p1, int indexP1, const Sequence* p2, int indexP2) {
	int dr = p1->r[indexP1] - p2->r[indexP2];
	int dg = p1->g[indexP1] - p2->g[indexP2];
	int db = p1->b[indexP1] - p2->b[indexP2];
	double d = sqrt(dr * dr + dg * dg + db * db);
	return (MAX_PIXEL_DISTANCE - superModerate(d, MAX_PIXEL_DISTANCE, MAX_PIXEL_DISTANCE / 2)) / MAX_PIXEL_DISTANCE;
}

inline double compare(const Sequence* p1, const Sequence* p2, int width, int height) {
	double totalScore = 0;
	int size = width * height;
	for (int index = 0; index < size; ++index) {
		int x = index % width;
		int y = index / width;
		int xMin = std::max(0, x - 1);
		int xMax = std::min(x + 1, width - 1);
		int yMin = std::max(0, y - 1);
		int yMax = std::min(y + 1, height - 1);
		int localWidth = xMax - xMin + 1;
		int localHeight = yMax - yMin + 1;
		int localSize = localWidth * localHeight;
		double score = -1;
		for (int localIndex = 0; localIndex < localSize; ++localIndex) {
			score = std::max(score, pixelSimilarity(p1, index, p2, (yMin + localIndex / localWidth) * width + xMin + localIndex % localWidth));
		}
		totalScore += score;
		/*
		for (int localY = yMin; localY <= yMax; ++localY) {
			for (int localX = xMin; localX <= xMax; ++localX) {
				score = std::max(score, pixelSimilarity(p1, x + y * width, p2, localX + localY * width));
			}
		}
		*/
	}
	return totalScore / size;
}

void sub(Sequence** sequences, int width, int height, double similarityLimit, int v, int i, int jFrom, int jTo) {
	// Aligner aligner(height, width, 0, v, 0);
	// double alignmentLimit = similarityLimit;
	for (int j = jFrom; j < jTo; ++j) {
		double score = compare(sequences[i], sequences[j], width, height);
		// std::cout << "D [" << i << " " << j << "] " << score << std::endl;
		if (score >= similarityLimit) {
			sequences[j]->classification = sequences[i]->classification;
			sequences[j]->score = score;
			/*
			score = aligner.align2(sequences[i], sequences[j]);
			std::cout << "\tS [" << i << " " << j << "] " << score << std::endl;
			if (score >= alignmentLimit) {
				sequences[j]->classification = sequences[i]->classification;
				sequences[j]->score = score;
			}
			*/
		}
	}
}

void classifySimilarities(Sequence** sequences, int nbSequences, int width, int height, double similarityLimit, int v) {
	std::cout << "STARTING" << std::endl;
	int nbThreads = 8;
	for (int i = 0; i < nbSequences - 1; ++i) {
		int a = i + 1;
		int l = (nbSequences - i - 1) / nbThreads;
		std::vector<std::thread> processes(nbThreads);
		for (int t = 0; t < nbThreads - 1; ++t) {
			processes[t] = std::thread(
					sub, sequences, width, height, similarityLimit, v, i, a + t * l, a + (t + 1) * l);
		}
		processes[nbThreads - 1] = std::thread(
				sub, sequences, width, height, similarityLimit, v, i, a + (nbThreads - 1) * l, nbSequences);
		for (auto& process: processes)
			process.join();
		if ((i + 1) % 100 == 0) {
			std::cout
				<< "(*) Image " << i + 1 << " / " << nbSequences
				<< " (" << l << " per thread on " << nbThreads << " threads)" << std::endl;
		}
	}
}