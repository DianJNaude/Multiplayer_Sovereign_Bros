#ifndef MULTIPLAYERMANAGER_H
#define MULTIPLAYERMANAGER_H

#include "Playerguy.h"

#include <QList>
#include <QRectF>

class MultiplayerManager
{
public:
    static const int TeleportDurationFrames = 180;
    static const int HeadBounceAssistPoints = 15;
    static const int CompletionBonusPoints = 100;

    void reset();
    void tickTeleportTimers(const QList<PlayerGuy*> &players);
    bool tryHeadBounce(PlayerGuy *fallingPlayer, int fallingIndex, PlayerGuy *lowerPlayer, int lowerIndex);
    bool resolveSideCollision(PlayerGuy *playerA, PlayerGuy *playerB);

private:
    bool isLandingOnHead(const QRectF &fallingRect, const QRectF &lowerRect, double fallingVelocityY) const;
};

#endif
