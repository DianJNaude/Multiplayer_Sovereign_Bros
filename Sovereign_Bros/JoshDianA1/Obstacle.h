#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <QGraphicsRectItem>

class PlayerGuy;

class Obstacle : public QGraphicsRectItem
{
public:
    enum Type
    {
        BouncePad
    };

    Obstacle(Type type, double width, double height, QGraphicsItem *parent = nullptr);

    Type getType() const;

    void triggerEffect(PlayerGuy *player);

private:
    Type obstacleType;
};

#endif
