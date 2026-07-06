#include "levelmanager.h"

#include <QtGlobal>

LevelManager::LevelManager()
    : level(1),
      currentLevelFrames(0),
      completedLevelFrames(3, 0)
{
}

void LevelManager::reset()
{
    level = 1;
    currentLevelFrames = 0;
    completedLevelFrames = QVector<int>(3, 0);
}

int LevelManager::currentLevel() const
{
    return level;
}

void LevelManager::setCurrentLevel(int levelNumber)
{
    level = levelNumber;
    currentLevelFrames = 0;
}

bool LevelManager::hasNextLevel() const
{
    return level < 2;
}

void LevelManager::advanceLevel()
{
    if (hasNextLevel())
    {
        level++;
        currentLevelFrames = 0;
    }
}

void LevelManager::startLevelTimer()
{
    currentLevelFrames = 0;
}

void LevelManager::tick()
{
    currentLevelFrames++;
}

void LevelManager::finishCurrentLevel()
{
    if (level >= 0 && level < completedLevelFrames.size())
    {
        completedLevelFrames[level] = currentLevelFrames;
    }
}

void LevelManager::setNetworkTiming(double currentSeconds, double levelOneSeconds, double levelTwoSeconds)
{
    currentLevelFrames = qRound(currentSeconds * 60.0);

    if (completedLevelFrames.size() < 3)
    {
        completedLevelFrames = QVector<int>(3, 0);
    }

    completedLevelFrames[1] = qRound(levelOneSeconds * 60.0);
    completedLevelFrames[2] = qRound(levelTwoSeconds * 60.0);
}

double LevelManager::currentLevelSeconds() const
{
    return currentLevelFrames / 60.0;
}

double LevelManager::levelSeconds(int levelNumber) const
{
    if (levelNumber >= 0 && levelNumber < completedLevelFrames.size())
    {
        return completedLevelFrames[levelNumber] / 60.0;
    }

    return 0.0;
}

double LevelManager::totalSeconds() const
{
    return levelSeconds(1) + levelSeconds(2);
}
