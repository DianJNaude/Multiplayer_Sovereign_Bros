#ifndef PLAYERGUY_H
#define PLAYERGUY_H

#include <QColor>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QList>

class PlayerGuy : public QGraphicsRectItem
{
public:
    PlayerGuy(int playerNumber = 1, int level = 1, QGraphicsItem *parent = nullptr);

    void updatePhysics(const QList<QGraphicsRectItem*> &platforms);

    bool jump();
    void bounce();
    void friendBounce();

    void moveLeft();
    void moveRight();
    void moveLeftSlow();
    void moveRightSlow();
    void stopMoving();
    void resetPosition(const QPointF &spawnPoint);
    void resetPosition();

    int getFacingDirection() const;
    int getPlayerNumber() const;
    double getVelocityY() const;
    bool isOnGround() const;
    void setNetworkState(const QPointF &position, int direction, int healthValue, bool shield);

    void setShieldActive(bool active);
    bool isShieldActive() const;
    QRectF getShieldSceneRect() const;

    QPointF getGunPoint() const;
    QPointF getSpawnPoint() const;

    void takeDamage(int amount = 1);
    void setHealth(int healthValue);
    void restoreHealth();
    int getHealth() const;
    bool isDead() const;

    void enableTeleportTo(int targetPlayerIndex, int durationFrames);
    void disableTeleport();
    void tickTeleportTimer();
    bool canTeleport() const;
    int getTeleportTargetPlayerIndex() const;
    int getTeleportFramesRemaining() const;

private:
    bool isCollidingWithAnyPlatform(const QList<QGraphicsRectItem*> &platforms);
    void updateShieldPosition();
    void updateHealthLabel();

private:
    QGraphicsTextItem *playerLabel;
    QGraphicsTextItem *healthLabel;
    QGraphicsRectItem *gunItem;
    QGraphicsRectItem *shieldItem;
    QGraphicsPixmapItem *playerImage;

    double velocityX;
    double velocityY;

    const double moveSpeed = 5.0;
    const double jumpStrength = -15.0;
    const double bounceStrength = -17.9;
    const double jumpPadBounceStrength = -25.2;

    const double gravity = 0.8;
    const double maxFallSpeed = 18.0;

    bool onGround;
    bool shieldActive;

    int facingDirection;
    int bounceCooldown;
    int playerNumber;
    int health;
    int maxHealth;
    bool teleportReady;
    int teleportTargetPlayerIndex;
    int teleportFramesRemaining;

    QPointF spawnPoint;
};

#endif
