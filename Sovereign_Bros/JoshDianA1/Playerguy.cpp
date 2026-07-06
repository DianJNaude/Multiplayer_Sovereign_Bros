#include "playerguy.h"

#include <QBrush>
#include <QPen>
#include <QFont>
#include <QtGlobal>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include "imageutils.h"

PlayerGuy::PlayerGuy(int playerNumber, int level, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      playerLabel(nullptr),
      healthLabel(nullptr),
      gunItem(nullptr),
      shieldItem(nullptr),
      velocityX(0),
      velocityY(0),
      onGround(false),
      shieldActive(false),
      facingDirection(1),
      bounceCooldown(0),
      playerNumber(playerNumber),
      health(3),
      maxHealth(3),
      teleportReady(false),
      teleportTargetPlayerIndex(-1),
      teleportFramesRemaining(0),
      spawnPoint(50, 472)
{
    setRect(0, 0, 90, 108);
    setBrush(Qt::transparent);
    setPen(Qt::NoPen);
    
    QString imageFile;
    if (playerNumber == 1) {
        if (level == 1) imageFile = "Player1.png";
        else imageFile = "Player1_2.png";
    } else {
        if (level == 1) imageFile = "Player2.png";
        else imageFile = "Player2_2.png";
    }

    QImage img(":/GUI/" + imageFile);
    img = cropTransparent(img);
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    playerImage = new QGraphicsPixmapItem(pix, this);
    
    setZValue(50);

    playerLabel = new QGraphicsTextItem(QString("PLAYER %1").arg(playerNumber), this);
    playerLabel->setDefaultTextColor(Qt::black);
    playerLabel->setFont(QFont("Arial", 12, QFont::Bold));
    playerLabel->setPos((rect().width() - playerLabel->boundingRect().width()) / 2, -34);

    healthLabel = new QGraphicsTextItem(this);
    healthLabel->setDefaultTextColor(Qt::darkRed);
    healthLabel->setFont(QFont("Arial", 10, QFont::Bold));
    healthLabel->setPos((rect().width() - healthLabel->boundingRect().width()) / 2, -18);

    gunItem = new QGraphicsRectItem(0, 0, 30, 8, this);
    gunItem->setBrush(QBrush(Qt::black));
    gunItem->setPen(QPen(Qt::NoPen));
    gunItem->setZValue(60);
    gunItem->hide();

    shieldItem = new QGraphicsRectItem(0, 0, 14, 70, this);
    shieldItem->setBrush(QBrush(QColor(0, 180, 255, 150)));
    shieldItem->setPen(QPen(Qt::blue, 2));
    shieldItem->setZValue(70);
    shieldItem->hide();

    updateShieldPosition();
    updateHealthLabel();
    resetPosition(spawnPoint);
}

void PlayerGuy::moveLeft()
{
    velocityX = -moveSpeed;
    facingDirection = -1;
    updateShieldPosition();
}

void PlayerGuy::moveRight()
{
    velocityX = moveSpeed;
    facingDirection = 1;
    updateShieldPosition();
}

void PlayerGuy::stopMoving()
{
    velocityX = 0;
}

bool PlayerGuy::jump()
{
    if (onGround)
    {
        velocityY = jumpStrength;
        onGround = false;
        return true;
    }

    return false;
}

void PlayerGuy::bounce()
{
    if (bounceCooldown <= 0)
    {
        velocityY = jumpPadBounceStrength;
        onGround = false;
        bounceCooldown = 18;
    }
}

void PlayerGuy::friendBounce()
{
    velocityY = bounceStrength;
    onGround = false;
    bounceCooldown = 12;
}

int PlayerGuy::getFacingDirection() const
{
    return facingDirection;
}

int PlayerGuy::getPlayerNumber() const
{
    return playerNumber;
}

double PlayerGuy::getVelocityY() const
{
    return velocityY;
}

bool PlayerGuy::isOnGround() const
{
    return onGround;
}

void PlayerGuy::setNetworkState(const QPointF &position, int direction, int healthValue, bool shield)
{
    setPos(position);
    velocityX = 0;
    velocityY = 0;
    facingDirection = direction < 0 ? -1 : 1;
    health = qMax(0, qMin(maxHealth, healthValue));
    shieldActive = shield;

    if (shieldActive)
    {
        shieldItem->show();
    }
    else
    {
        shieldItem->hide();
    }

    updateShieldPosition();
    updateHealthLabel();
}

void PlayerGuy::setShieldActive(bool active)
{
    shieldActive = active;

    if (shieldActive)
    {
        shieldItem->show();
    }
    else
    {
        shieldItem->hide();
    }

    updateShieldPosition();
}

bool PlayerGuy::isShieldActive() const
{
    return shieldActive;
}

QRectF PlayerGuy::getShieldSceneRect() const
{
    return shieldItem->sceneBoundingRect();
}

QPointF PlayerGuy::getGunPoint() const
{
    if (facingDirection == 1)
    {
        return mapToScene(rect().width() + 30, rect().height() * 0.46 + 4);
    }
    else
    {
        return mapToScene(-30, rect().height() * 0.46 + 4);
    }
}

QPointF PlayerGuy::getSpawnPoint() const
{
    return spawnPoint;
}

void PlayerGuy::takeDamage(int amount)
{
    health -= amount;

    if (health < 0)
    {
        health = 0;
    }

    updateHealthLabel();
}

void PlayerGuy::setHealth(int healthValue)
{
    health = qMax(0, qMin(maxHealth, healthValue));
    updateHealthLabel();
}

void PlayerGuy::restoreHealth()
{
    health = maxHealth;
    updateHealthLabel();
}

int PlayerGuy::getHealth() const
{
    return health;
}

bool PlayerGuy::isDead() const
{
    return health <= 0;
}

void PlayerGuy::enableTeleportTo(int targetPlayerIndex, int durationFrames)
{
    teleportReady = true;
    teleportTargetPlayerIndex = targetPlayerIndex;
    teleportFramesRemaining = durationFrames;
}

void PlayerGuy::disableTeleport()
{
    teleportReady = false;
    teleportTargetPlayerIndex = -1;
    teleportFramesRemaining = 0;
}

void PlayerGuy::tickTeleportTimer()
{
    if (!teleportReady)
        return;

    teleportFramesRemaining--;

    if (teleportFramesRemaining <= 0)
    {
        disableTeleport();
    }
}

bool PlayerGuy::canTeleport() const
{
    return teleportReady;
}

int PlayerGuy::getTeleportTargetPlayerIndex() const
{
    return teleportTargetPlayerIndex;
}

int PlayerGuy::getTeleportFramesRemaining() const
{
    return teleportFramesRemaining;
}

void PlayerGuy::updateShieldPosition()
{
    if (!gunItem || !shieldItem)
        return;

    if (facingDirection == 1)
    {
        if (playerImage) playerImage->setTransform(QTransform());
        
        gunItem->setRect(0, 0, 30, 8);
        gunItem->setPos(rect().width() - 6, rect().height() * 0.46);

        shieldItem->setPos(rect().width() + 33, -5);
    }
    else
    {
        if (playerImage) {
            QTransform t;
            t.translate(rect().width(), 0);
            t.scale(-1, 1);
            playerImage->setTransform(t);
        }
        
        gunItem->setRect(0, 0, 30, 8);
        gunItem->setPos(-30, rect().height() * 0.46);

        shieldItem->setPos(-57, -5);
    }
}

void PlayerGuy::updateHealthLabel()
{
    if (healthLabel)
    {
        healthLabel->setPlainText(QString("HP: %1").arg(health));
    }
}

bool PlayerGuy::isCollidingWithAnyPlatform(const QList<QGraphicsRectItem*> &platforms)
{
    QRectF playerRect = sceneBoundingRect();

    for (QGraphicsRectItem *platform : platforms)
    {
        if (playerRect.intersects(platform->sceneBoundingRect()))
        {
            return true;
        }
    }

    return false;
}

void PlayerGuy::updatePhysics(const QList<QGraphicsRectItem*> &platforms)
{
    if (bounceCooldown > 0)
    {
        bounceCooldown--;
    }

    updateShieldPosition();

    double oldX = x();

    setPos(x() + velocityX, y());

    if (isCollidingWithAnyPlatform(platforms))
    {
        setPos(oldX, y());
        velocityX = 0;
    }

    velocityY += gravity;

    if (velocityY > maxFallSpeed)
    {
        velocityY = maxFallSpeed;
    }

    setPos(x(), y() + velocityY);
    onGround = false;

    QRectF playerRect = sceneBoundingRect();

    for (QGraphicsRectItem *platform : platforms)
    {
        QRectF platformRect = platform->sceneBoundingRect();

        if (playerRect.intersects(platformRect))
        {
            if (velocityY > 0)
            {
                setPos(x(), platformRect.top() - rect().height());
                velocityY = 0;
                onGround = true;
            }
            else if (velocityY < 0)
            {
                setPos(x(), platformRect.bottom());
                velocityY = 0;
            }

            playerRect = sceneBoundingRect();
        }
    }

    if (x() < -400)
    {
        setPos(-400, y());
    }
}

void PlayerGuy::resetPosition(const QPointF &newSpawnPoint)
{
    spawnPoint = newSpawnPoint;
    resetPosition();
}

void PlayerGuy::resetPosition()
{
    setPos(spawnPoint);
    velocityX = 0;
    velocityY = 0;
    onGround = false;
    shieldActive = false;
    shieldItem->hide();
    disableTeleport();
    restoreHealth();
    updateShieldPosition();
}

void PlayerGuy::moveLeftSlow()
{
    velocityX = -moveSpeed * 0.35;
    facingDirection = -1;
    updateShieldPosition();
}

void PlayerGuy::moveRightSlow()
{
    velocityX = moveSpeed * 0.35;
    facingDirection = 1;
    updateShieldPosition();
}
