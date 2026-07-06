#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include <QVector>

class LevelManager
{
public:
    LevelManager();

    void reset();
    int currentLevel() const;
    void setCurrentLevel(int level);
    bool hasNextLevel() const;
    void advanceLevel();

    void startLevelTimer();
    void tick();
    void finishCurrentLevel();
    void setNetworkTiming(double currentSeconds, double levelOneSeconds, double levelTwoSeconds);

    double currentLevelSeconds() const;
    double levelSeconds(int level) const;
    double totalSeconds() const;

private:
    int level;
    int currentLevelFrames;
    QVector<int> completedLevelFrames;
};

#endif
