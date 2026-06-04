#include "ScoreManager.h"

ScoreManager::ScoreManager() { };
ScoreManager::~ScoreManager() { };

int ScoreManager::getScore(int team) const {
	if (team == 1) return _score1;
	if (team == 2) return _score2;
	return -1;
};

void ScoreManager::setScore(int team, int score) {
	if (score < 0) return;
	if (team == 1) _score1 = score;
	if (team == 2) _score2 = score;
};

void ScoreManager::incrimentScore(int team) {
	if (team == 1) _score1++;
	if (team == 2) _score2++;
};

void ScoreManager::resetAll() {
	_score1 = 0;
	_score2 = 0;
};