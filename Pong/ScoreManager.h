#pragma once

class ScoreManager {
	private:
		int _score1{0};
		int _score2{0};

	public:
		ScoreManager();
		~ScoreManager();

		int getScore(int team) const;
		void setScore(int team, int score);

		void incrimentScore(int team);

		void resetAll();
};