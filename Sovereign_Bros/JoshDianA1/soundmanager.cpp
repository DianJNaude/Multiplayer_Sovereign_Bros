#include "soundmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>

SoundManager::SoundManager(QObject *parent)
    : QObject(parent)
{
    setupEffect(&shootEffect, "shoot.wav", 0.45);
    setupEffect(&jumpEffect, "jump.wav", 0.45);
    setupEffect(&coinEffect, "coin.wav", 0.55);
    setupEffect(&levelCompleteEffect, "level_complete.wav", 0.55);
    setupEffect(&bounceEffect, "bounce.wav", 0.55);

    // Grenade explosion sound
    setupEffect(&explosionEffect, "grenade.wav", 0.80);

    shootEffect.setVolume(0);
    shootEffect.play();

    jumpEffect.setVolume(0);
    jumpEffect.play();

    coinEffect.setVolume(0);
    coinEffect.play();

    levelCompleteEffect.setVolume(0);
    levelCompleteEffect.play();

    explosionEffect.setVolume(0);
    explosionEffect.play();

    bounceEffect.setVolume(0);
    bounceEffect.play();

    QTimer::singleShot(1000, this, [this]() {
        shootEffect.setVolume(0.45);
        jumpEffect.setVolume(0.45);
        coinEffect.setVolume(0.55);
        levelCompleteEffect.setVolume(0.55);
        explosionEffect.setVolume(0.80);
        bounceEffect.setVolume(0.55);
    });
}
void SoundManager::playShoot()
{
    playEffect(&shootEffect);
}

void SoundManager::playGrenade()
{

}

void SoundManager::playExplosion()
{
    qDebug() << "Playing grenade explosion sound:"
             << explosionEffect.source()
             << "status:" << explosionEffect.status();

    explosionEffect.stop();
    explosionEffect.play();
}

void SoundManager::playJump()
{
    playEffect(&jumpEffect);
}

void SoundManager::playBounce()
{
    if (bounceEffect.source().isEmpty())
    {
        playJump();
        return;
    }

    playEffect(&bounceEffect);
}

void SoundManager::playCoin()
{
    playEffect(&coinEffect);
}

void SoundManager::playLevelComplete()
{
    playEffect(&levelCompleteEffect);
}

void SoundManager::setupEffect(QSoundEffect *effect, const QString &fileName, qreal volume)
{
    effect->setSource(QUrl("qrc:/sounds/" + fileName));
    effect->setLoopCount(1);
    effect->setVolume(volume);
}

void SoundManager::playEffect(QSoundEffect *effect)
{
    if (!effect || effect->source().isEmpty())
        return;

    effect->play();
}
