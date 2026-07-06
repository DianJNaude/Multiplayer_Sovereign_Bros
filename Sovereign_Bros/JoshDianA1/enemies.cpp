#include "enemies.h"
#include <QGraphicsScene>
#include <QBrush>
#include <QPen>
#include <QGraphicsPolygonItem>
#include <QtMath>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include "imageutils.h"

BaseEnemy::BaseEnemy(EnemyKind kind, int scoreValue, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      speed(0),
      direction(1),
      moveCounter(0),
      patrolFrames(90),
      startY(0),
      shootingEnemy(false),
      kind(kind),
      scoreValue(scoreValue)
{
    setPen(QPen(Qt::NoPen));
    setBrush(QBrush(Qt::transparent));
    setZValue(10);
}

BaseEnemy::~BaseEnemy()
{
}

void BaseEnemy::updateBehavior()
{
}

void BaseEnemy::updateDirectionVisual()
{
    if (direction == -1) {
        setTransform(QTransform());
    } else {
        QTransform t;
        t.translate(rect().width(), 0);
        t.scale(-1, 1);
        setTransform(t);
    }
}

int BaseEnemy::getDirection() const
{
    return direction;
}

QPointF BaseEnemy::getGunPoint() const
{
    return mapToScene(rect().width(), rect().height() * 0.40);
}

BaseEnemy::EnemyKind BaseEnemy::getKind() const
{
    return kind;
}

int BaseEnemy::getScoreValue() const
{
    return scoreValue;
}

bool BaseEnemy::canShoot() const
{
    return shootingEnemy;
}

QString BaseEnemy::displayName() const
{
    switch (kind)
    {
    case Fish: return "Fish";
    case BigFish: return "Big fish";
    case Bird: return "Killer bird";
    case Plant: return "Deadly plant";
    case Shark: return "Shark";
    case Octopus: return "Octopus";
    }

    return "Enemy";
}

void BaseEnemy::buildFishEnemyVisual(const QColor &bodyColor, const QColor &legColor, bool bigEnemy)
{
    Q_UNUSED(legColor);
    Q_UNUSED(bigEnemy);

    QImage img(":/GUI/walkenemy.png");
    img = cropTransparent(img);

    if (bodyColor == Qt::magenta)
    {
        // Tint it pink!
        QImage tintedImg = img.convertToFormat(QImage::Format_ARGB32);
        QColor pink(255, 105, 180);
        for (int y = 0; y < tintedImg.height(); ++y)
        {
            for (int x = 0; x < tintedImg.width(); ++x)
            {
                QRgb pixel = tintedImg.pixel(x, y);
                int alpha = qAlpha(pixel);
                if (alpha > 0)
                {
                    int r = qRed(pixel);
                    int g = qGreen(pixel);
                    int b = qBlue(pixel);
                    int gray = 0.299 * r + 0.587 * g + 0.114 * b;
                    int tr = (gray * pink.red()) / 255;
                    int tg = (gray * pink.green()) / 255;
                    int tb = (gray * pink.blue()) / 255;
                    tintedImg.setPixel(x, y, qRgba(tr, tg, tb, alpha));
                }
            }
        }
        img = tintedImg;
    }

    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    new QGraphicsPixmapItem(pix, this);
}

void BaseEnemy::buildBirdVisual()
{
    QImage img(":/GUI/Enemy.png");
    img = cropTransparent(img);
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    new QGraphicsPixmapItem(pix, this);
}

void BaseEnemy::buildPlantVisual()
{
    QImage img(":/GUI/flower.png");
    img = cropTransparent(img);
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    new QGraphicsPixmapItem(pix, this);
}

void BaseEnemy::buildSharkVisual()
{
    QImage img(":/GUI/enemyfish.png");
    img = cropTransparent(img);
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    new QGraphicsPixmapItem(pix, this);
}

void BaseEnemy::buildOctopusVisual()
{
    QImage img(":/GUI/jellyfish.png");
    img = cropTransparent(img);
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(rect().width(), rect().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    new QGraphicsPixmapItem(pix, this);
}

FishEnemy::FishEnemy(QGraphicsItem *parent)
    : BaseEnemy(Fish, 25, parent)
{
    setRect(0, 0, 55, 55);
    speed = 4.2;
    direction = 1;
    patrolFrames = 45;
    buildFishEnemyVisual(Qt::magenta, Qt::yellow, false);
}

void FishEnemy::updateBehavior()
{
    moveBy(speed * direction, 0);
    moveCounter++;

    if (moveCounter > patrolFrames)
    {
        direction *= -1;
        moveCounter = 0;
        updateDirectionVisual();
    }
}

FastEnemy::FastEnemy(QGraphicsItem *parent)
    : FishEnemy(parent)
{
}

BigEnemy::BigEnemy(QGraphicsItem *parent)
    : BaseEnemy(BigFish, 25, parent)
{
    setRect(0, 0, 100, 95);
    speed = 1.4;
    direction = 1;
    patrolFrames = 150;
    buildFishEnemyVisual(Qt::darkRed, Qt::darkYellow, true);
}

void BigEnemy::updateBehavior()
{
    moveBy(speed * direction, 0);
    moveCounter++;

    if (moveCounter > patrolFrames)
    {
        direction *= -1;
        moveCounter = 0;
        updateDirectionVisual();
    }
}

BirdEnemy::BirdEnemy(QGraphicsItem *parent)
    : BaseEnemy(Bird, 40, parent)
{
    setRect(0, 0, 75, 60);
    speed = 1.8;
    direction = 1;
    patrolFrames = 140;
    startY = 0;
    buildBirdVisual();
}

void BirdEnemy::updateBehavior()
{
    if (moveCounter == 0)
    {
        startY = y();
    }

    moveBy(speed * direction, 0);
    moveCounter++;
    setY(startY + qSin(moveCounter * 0.10) * 24.0);

    if (moveCounter % patrolFrames == 0)
    {
        direction *= -1;
        updateDirectionVisual();
    }
}

PlantEnemy::PlantEnemy(QGraphicsItem *parent)
    : BaseEnemy(Plant, 30, parent)
{
    setRect(0, 0, 60, 80);
    speed = 0;
    buildPlantVisual();
}

void PlantEnemy::updateBehavior()
{
}

SharkEnemy::SharkEnemy(QGraphicsItem *parent)
    : BaseEnemy(Shark, 50, parent)
{
    setRect(0, 0, 105, 58);
    speed = 3.0;
    direction = 1;
    patrolFrames = 95;
    startY = 0;
    buildSharkVisual();
}

void SharkEnemy::updateBehavior()
{
    if (moveCounter == 0)
    {
        startY = y();
    }

    moveBy(speed * direction, qSin(moveCounter * 0.08) * 0.9);
    moveCounter++;

    if (moveCounter > patrolFrames)
    {
        direction *= -1;
        moveCounter = 0;
        setY(startY);
        updateDirectionVisual();
    }
}

OctopusEnemy::OctopusEnemy(QGraphicsItem *parent)
    : BaseEnemy(Octopus, 75, parent)
{
    setRect(0, 0, 80, 82);
    speed = 1.1;
    direction = -1;
    patrolFrames = 120;
    shootingEnemy = true;
    buildOctopusVisual();
}

void OctopusEnemy::updateBehavior()
{
    moveBy(speed * direction, 0);
    moveCounter++;

    if (moveCounter > patrolFrames)
    {
        direction *= -1;
        moveCounter = 0;
        updateDirectionVisual();
    }
}


FlyingFishEnemy::FlyingFishEnemy(QGraphicsItem *parent)
    : BaseEnemy(FlyingFish, 60, parent),
      targetPosition(0, 0),
      velocityX(4.0),
      velocityY(3.0)
{
    setRect(0, 0, 70, 45);

    speed = 4.0;
    direction = 1;
    patrolFrames = 999999;

    // Use the same fish image, but as a flying fish enemy
    buildSharkVisual();
}

void FlyingFishEnemy::setTargetPosition(const QPointF &target)
{
    targetPosition = target;
}

void FlyingFishEnemy::updateBehavior()
{
    QRectF bounds;

    if (scene())
    {
        bounds = scene()->sceneRect();
    }
    else
    {
        bounds = QRectF(0, 0, 3000, 650);
    }

    QPointF center = sceneBoundingRect().center();

    double dx = targetPosition.x() - center.x();
    double dy = targetPosition.y() - center.y();

    double length = qSqrt(dx * dx + dy * dy);

    if (length > 1.0)
    {
        dx /= length;
        dy /= length;

        // Small chase force toward player
        velocityX += dx * 0.22;
        velocityY += dy * 0.22;
    }

    // Limit speed so it does not become impossible
    double currentSpeed = qSqrt(velocityX * velocityX + velocityY * velocityY);
    double maxSpeed = 6.0;

    if (currentSpeed > maxSpeed)
    {
        velocityX = (velocityX / currentSpeed) * maxSpeed;
        velocityY = (velocityY / currentSpeed) * maxSpeed;
    }

    moveBy(velocityX, velocityY);

    QRectF r = sceneBoundingRect();

    // Bounce from left and right sides
    if (r.left() < bounds.left())
    {
        setX(bounds.left());
        velocityX = qAbs(velocityX);
    }
    else if (r.right() > bounds.right())
    {
        setX(bounds.right() - rect().width());
        velocityX = -qAbs(velocityX);
    }

    // Bounce from top and ground area
    if (r.top() < 80)
    {
        setY(80);
        velocityY = qAbs(velocityY);
    }
    else if (r.bottom() > 560)
    {
        setY(560 - rect().height());
        velocityY = -qAbs(velocityY);
    }

    if (velocityX < 0)
    {
        direction = -1;
    }
    else
    {
        direction = 1;
    }

    updateDirectionVisual();
}
