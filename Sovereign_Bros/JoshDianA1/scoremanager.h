#ifndef SCOREMANAGER_H
#define SCOREMANAGER_H

#include <QString>
#include <QVector>

struct PlayerStats
{
    QString name;
    int score;
    int coinsCollected;
    int enemiesKilled;

    PlayerStats(const QString &name = QString())
        : name(name),
          score(0),
          coinsCollected(0),
          enemiesKilled(0)
    {
    }
};

class ScoreManager
{
public:
    void reset(int playerCount);
    void addCoin(int playerIndex, int points);
    void addEnemyKill(int playerIndex, int points);
    void addBonus(int playerIndex, int points);
    void setStatsForPlayer(int playerIndex, const PlayerStats &playerStats);

    PlayerStats statsForPlayer(int playerIndex) const;
    int playerCount() const;
    int teamScore() const;

private:
    QVector<PlayerStats> stats;
};

#endif
