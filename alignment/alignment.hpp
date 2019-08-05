//
// Created by notoraptor on 27/06/2019.
//

#ifndef VIDEORAPTOR_ALIGNMENT_HPP
#define VIDEORAPTOR_ALIGNMENT_HPP

struct Sequence {
	int* r; // red
	int* g; // green
	int* b; // blue
	int* i; // gray
	double score;
	int classification;
};


extern "C" {
	double batchAlignmentScore(
			const int* A, const int* B, int rows, int columns, int minVal, int maxVal, int gapScore);
	void classifySimilarities(
			Sequence** sequences, int nbSequences, int width, int height, double* edges);
};

#endif //VIDEORAPTOR_ALIGNMENT_HPP
