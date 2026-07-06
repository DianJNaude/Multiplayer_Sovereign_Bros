#include "obstacle.h"
#include "playerguy.h"

#include <QBrush>
#include <QPen>

Obstacle::Obstacle(Type type, double width, double height, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      obstacleType(type)
{
    setRect(0, 0, width, height);
    setPen(QPen(Qt::black, 2));
    setZValue(5);

    if (type == BouncePad)
    {
        setBrush(QBrush(Qt::green));
    }
}

Obstacle::Type Obstacle::getType() const
{
    return obstacleType;
}

void Obstacle::triggerEffect(PlayerGuy *player)
{
    if (!player)
        return;

    if (obstacleType == BouncePad)
    {
        player->bounce();
    }
}
