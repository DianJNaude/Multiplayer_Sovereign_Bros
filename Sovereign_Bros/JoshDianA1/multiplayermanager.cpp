#include "multiplayermanager.h"

#include <QtMath>

void MultiplayerManager::reset()
{
}

void MultiplayerManager::tickTeleportTimers(const QList<PlayerGuy*> &players)
{
    for (PlayerGuy *player : players)
    {
        if (player)
        {
            player->tickTeleportTimer();
        }
    }
}

bool MultiplayerManager::tryHeadBounce(PlayerGuy *fallingPlayer, int fallingIndex, PlayerGuy *lowerPlayer, int lowerIndex)
{
    if (!fallingPlayer || !lowerPlayer)
        return false;

    if (!isLandingOnHead(fallingPlayer->sceneBoundingRect(),
                         lowerPlayer->sceneBoundingRect(),
                         fallingPlayer->getVelocityY()))
    {
        return false;
    }

    fallingPlayer->setY(lowerPlayer->y() - fallingPlayer->rect().height() - 1);
    fallingPlayer->friendBounce();
    lowerPlayer->enableTeleportTo(fallingIndex, TeleportDurationFrames);
    Q_UNUSED(lowerIndex);

    return true;
}

bool MultiplayerManager::resolveSideCollision(PlayerGuy *playerA, PlayerGuy *playerB)
{
    if (!playerA || !playerB)
        return false;

    QRectF rectA = playerA->sceneBoundingRect();
    QRectF rectB = playerB->sceneBoundingRect();

    if (!rectA.intersects(rectB))
        return false;

    double overlapLeft = rectA.right() - rectB.left();
    double overlapRight = rectB.right() - rectA.left();
    double horizontalOverlap = qMin(overlapLeft, overlapRight);
    double verticalOverlap = qMin(rectA.bottom() - rectB.top(), rectB.bottom() - rectA.top());

    if (horizontalOverlap > 0 && horizontalOverlap < verticalOverlap)
    {
        double push = horizontalOverlap / 2.0 + 1.0;

        if (rectA.center().x() < rectB.center().x())
        {
            playerA->moveBy(-push, 0);
            playerB->moveBy(push, 0);
        }
        else
        {
            playerA->moveBy(push, 0);
            playerB->moveBy(-push, 0);
        }

        return true;
    }

    return false;
}

bool MultiplayerManager::isLandingOnHead(const QRectF &fallingRect, const QRectF &lowerRect, double fallingVelocityY) const
{
    if (fallingVelocityY <= 0.5)
        return false;

    if (!fallingRect.intersects(lowerRect))
        return false;

    double bottom = fallingRect.bottom();
    double top = lowerRect.top();
    bool nearTop = bottom >= top && bottom <= top + 22.0;
    bool centeredEnough = fallingRect.center().x() >= lowerRect.left() - 8.0 &&
                          fallingRect.center().x() <= lowerRect.right() + 8.0;

    return nearTop && centeredEnough;
}
