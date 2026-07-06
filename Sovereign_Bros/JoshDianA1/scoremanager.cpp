#include "scoremanager.h"

void ScoreManager::reset(int playerCount)
{
    stats.clear();

    for (int i = 0; i < playerCount; i++)
    {
        stats.append(PlayerStats(QString("Player %1").arg(i + 1)));
    }
}

void ScoreManager::addCoin(int playerIndex, int points)
{
    if (playerIndex < 0 || playerIndex >= stats.size())
        return;

    stats[playerIndex].coinsCollected++;
    stats[playerIndex].score += points;
}

void ScoreManager::addEnemyKill(int playerIndex, int points)
{
    if (playerIndex < 0 || playerIndex >= stats.size())
        return;

    stats[playerIndex].enemiesKilled++;
    stats[playerIndex].score += points;
}

void ScoreManager::addBonus(int playerIndex, int points)
{
    if (playerIndex < 0 || playerIndex >= stats.size())
        return;

    stats[playerIndex].score += points;
}

void ScoreManager::setStatsForPlayer(int playerIndex, const PlayerStats &playerStats)
{
    if (playerIndex < 0 || playerIndex >= stats.size())
        return;

    stats[playerIndex] = playerStats;
}

PlayerStats ScoreManager::statsForPlayer(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= stats.size())
    {
        return PlayerStats();
    }

    return stats[playerIndex];
}

int ScoreManager::playerCount() const
{
    return stats.size();
}

int ScoreManager::teamScore() const
{
    int total = 0;

    for (const PlayerStats &playerStats : stats)
    {
        total += playerStats.score;
    }

    return total;
}
