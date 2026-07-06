#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QSoundEffect>
#include <QMediaPlayer>

class SoundManager : public QObject
{
    Q_OBJECT

public:
    explicit SoundManager(QObject *parent = nullptr);

    void playShoot();
    void playGrenade();
    void playExplosion();
    void playJump();
    void playBounce();
    void playCoin();
    void playLevelComplete();

private:
    void setupEffect(QSoundEffect *effect, const QString &fileName, qreal volume);
    void playEffect(QSoundEffect *effect);

private:
    QSoundEffect shootEffect;
    QSoundEffect jumpEffect;
    QSoundEffect coinEffect;
    QSoundEffect levelCompleteEffect;
    QSoundEffect explosionEffect;
    QSoundEffect bounceEffect;
    QMediaPlayer grenadePlayer;
};

#endif
