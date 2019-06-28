//
// Created by notoraptor on 27/06/2019.
//

#ifndef VIDEORAPTOR_ALIGNMENT_HPP
#define VIDEORAPTOR_ALIGNMENT_HPP

struct Sequence {
	int* r;
	int* g;
	int* b;
	double score;
	int classification;
};


extern "C" {
	double batchAlignmentScore(const int* A, const int* B, int rows, int columns, int minVal, int maxVal, int gapScore);
	int classifySimilarities(
			Sequence** sequences, int nbSequences, double similarityLimit, double differenceLimit,
			int rows, int columns, int minVal, int maxVal, int gapScore);
};

#endif //VIDEORAPTOR_ALIGNMENT_HPP
