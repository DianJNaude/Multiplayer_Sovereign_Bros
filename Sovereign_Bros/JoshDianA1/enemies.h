#ifndef ENEMIES_H
#define ENEMIES_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QString>

class BaseEnemy : public QGraphicsRectItem
{
public:
    enum EnemyKind
    {
        Fish,
        BigFish,
        Bird,
        Plant,
        Shark,
        Octopus,
        FlyingFish
    };

    BaseEnemy(EnemyKind kind, int scoreValue, QGraphicsItem *parent = nullptr);
    virtual ~BaseEnemy();

    virtual void updateBehavior();

    int getDirection() const;
    QPointF getGunPoint() const;
    EnemyKind getKind() const;
    int getScoreValue() const;
    bool canShoot() const;
    QString displayName() const;

protected:
    void buildFishEnemyVisual(const QColor &bodyColor, const QColor &legColor, bool bigEnemy);
    void buildBirdVisual();
    void buildPlantVisual();
    void buildSharkVisual();
    void buildOctopusVisual();
    void updateDirectionVisual();

protected:
    double speed;
    int direction;
    int moveCounter;
    int patrolFrames;
    double startY;
    bool shootingEnemy;
    EnemyKind kind;
    int scoreValue;
};

class FishEnemy : public BaseEnemy
{
public:
    FishEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};

class FastEnemy : public FishEnemy
{
public:
    FastEnemy(QGraphicsItem *parent = nullptr);
};

class BigEnemy : public BaseEnemy
{
public:
    BigEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};

class BirdEnemy : public BaseEnemy
{
public:
    BirdEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};

class PlantEnemy : public BaseEnemy
{
public:
    PlantEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};

class SharkEnemy : public BaseEnemy
{
public:
    SharkEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};

class OctopusEnemy : public BaseEnemy
{
public:
    OctopusEnemy(QGraphicsItem *parent = nullptr);
    void updateBehavior() override;
};


class FlyingFishEnemy : public BaseEnemy
{
public:
    FlyingFishEnemy(QGraphicsItem *parent = nullptr);

    void setTargetPosition(const QPointF &target);
    void updateBehavior() override;

private:
    QPointF targetPosition;
    double velocityX;
    double velocityY;
};

#endif
