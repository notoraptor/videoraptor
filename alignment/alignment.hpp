//
// Created by notoraptor on 27/06/2019.
//

#ifndef VIDEORAPTOR_ALIGNMENT_HPP
#define VIDEORAPTOR_ALIGNMENT_HPP

struct Sequence {
	int* red;
	int* green;
	int* blue;
	double score;
	int classification;
};


extern "C" {
	double batchAlignmentScore(const int* A, const int* B, int rows, int columns, int minVal, int maxVal, int gapScore);
	void Sequence_init(Sequence* sequence);
	int classifySimilarities(
			Sequence** sequences, int nbSequences, double similarityLimit, double differenceLimit,
			int rows, int columns, int minVal, int maxVal, int gapScore);
};

#endif //VIDEORAPTOR_ALIGNMENT_HPP
