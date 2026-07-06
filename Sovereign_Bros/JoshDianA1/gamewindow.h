#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSet>
#include <QPointF>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>

#include <QGraphicsPolygonItem>

#include "Playerguy.h"
#include "enemies.h"
#include "Obstacle.h"
#include "scoremanager.h"
#include "levelmanager.h"
#include "multiplayermanager.h"
#include "networkmanager.h"
#include "soundmanager.h"

class GameWindow : public QGraphicsView
{
    Q_OBJECT

public:
    GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void gameLoop();
    void handleNetworkConnected();
    void handleNetworkDisconnected();
    void handleNetworkMessage(const QJsonObject &message);
    void handleNetworkError(const QString &error);
    void updateNetworkStatus(const QString &status);

private:

    QGraphicsPixmapItem *friendlyNpc;
    QGraphicsRectItem *friendlyTrigger;
    bool shieldUnlockedForLevel2;

    enum GameState
    {
        MainMenuState,
        PlayingState,
        ResultsState
    };

    enum NetworkGameMode
    {
        OfflineGame,
        HostGame,
        ClientGame
    };

    struct MenuButton
    {
        QGraphicsRectItem *rect;
        QGraphicsTextItem *label;
        QString action;
    };

    struct Projectile
    {
        QGraphicsRectItem *item;
        int ownerIndex;
        bool enemyProjectile;
    };

    struct Grenade
    {
        QGraphicsEllipseItem *item;
        int ownerIndex;
    };

    struct PlayerInputState
    {
        bool left = false;
        bool right = false;
        bool jump = false;
        bool shield = false;
    };

    void setupScene();
    void clearSceneData();

    void showMainMenu();
    void showMultiplayerMenu();
    void showConnectionScreen(const QString &message);
    void showHighScoresScreen();
    void showResultsScreen();
    void addMenuButton(const QString &text, const QString &action, double x, double y);
    void handleMenuClick(const QPointF &scenePoint);
    void handleResultsClick(const QPointF &scenePoint);

    void createSpikes(double x, double groundY, int spikeCount);

    void startGame(int playerCount);
    void startHostMultiplayer();
    void joinMultiplayer();
    void loadLevel(int levelNumber);

    void loadJungleLevel();
    void loadOceanLevel();
    void loadMultiplayerCoopLevel();
    void loadMultiplayerCoopOceanLevel();
    void completeLevel();

    void createFriendly(double x, double groundY);
    bool canUseShield() const;

    void setupPlayers(int playerCount);
    void createTree(double x, double y, bool isPlatform = false, bool isFullSolid = false);
    void createCloud(double x, double y);
    void createSeaweed(double x, double y);
    void createExit(double x, double y, const QColor &color);
    void createCoin(double x, double y);

    void createQuickSand(double x, double y, double w, double h);
    bool playerInQuickSand(PlayerGuy *player) const;

    void addEnemy(BaseEnemy *enemy, double x, double y);
    QGraphicsRectItem* addPlatform(double x, double y, double w, double h, const QColor &color);

    void updatePlayers();
    void updateCoopPlayerCollision();
    void updateEnemies();
    void updateProjectiles();
    void updateGrenades();
    void updateExplosions();
    void checkCollisions();
    void updateCamera();
    void updateHud();
    void sendClientInput();
    void applyInputToPlayer(int playerIndex, const PlayerInputState &input);
    QJsonObject buildInputMessage() const;
    QJsonObject buildGameStateMessage() const;
    void broadcastGameState();
    void applyGameState(const QJsonObject &message);
    void applyPlayerState(const QJsonArray &playersArray);
    void applyEnemyState(const QJsonArray &enemiesArray);
    void applyCoinState(const QJsonArray &coinsArray);
    void applyProjectileState(const QJsonArray &projectilesArray);
    void applyScoreState(const QJsonArray &scoresArray);
    void applyGrenadeState(const QJsonArray &grenadesArray);
    void applyExplosionState(const QJsonArray &explosionsArray);
    void sendPlayerAction(const QString &action);
    void handlePlayerAction(const QJsonObject &message);
    void playSoundEvent(const QString &soundName);
    void broadcastSoundEvent(const QString &soundName, int sourcePlayer = 0);
    void logResultsToFile();


    void shootPlayerBullet(int playerIndex);
    void shootEnemyBullet(BaseEnemy *enemy);

    void dropGrenade(int playerIndex);
    void explodeGrenade(int grenadeIndex);

    bool bulletHitsBlockingItem(QGraphicsRectItem *bullet) const;
    bool pointInsideExplosion(QPointF point, QPointF explosionCenter, double radius) const;
    bool rectIntersectsExplosion(const QRectF &rect, QPointF center, double radius) const;

    void damagePlayer(int playerIndex);
    bool tryTeleportPlayer(int playerIndex);
    QPointF findSafeTeleportPosition(PlayerGuy *teleportingPlayer, PlayerGuy *targetPlayer) const;
    bool positionIntersectsPlatform(PlayerGuy *player, const QPointF &position) const;
    void resetCurrentLevel();
    void resetPlayerToSpawn(int playerIndex);
    void removeProjectileAt(int index);
    void removeGrenadeAt(int index);
    void awardEnemyKill(int playerIndex, BaseEnemy *enemy);

private:
    QGraphicsScene *scene;
    QTimer *timer;
    QSet<int> keysPressed;

    GameState state;
    QList<MenuButton> menuButtons;

    QList<PlayerGuy*> players;
    QList<int> playerShootCooldowns;
    QList<int> grenadeCooldowns;
    QList<int> damageCooldowns;

    QGraphicsRectItem *finishFlag;
    QGraphicsTextItem *timerText;
    QGraphicsTextItem *scoreText;
    QGraphicsTextItem *controlsText;
    QGraphicsTextItem *levelText;

    QList<QGraphicsRectItem*> platforms;
    QList<QGraphicsRectItem*> blockingItems;
    QList<BaseEnemy*> enemies;
    QList<QGraphicsRectItem*> coins;
    QList<Obstacle*> obstacles;

    QList<QGraphicsRectItem*> quicksandAreas;

    QList<QGraphicsPolygonItem*> spikes;

    QList<Projectile> projectiles;
    QList<Grenade> grenades;
    QList<QGraphicsEllipseItem*> explosions;

    ScoreManager scoreManager;
    LevelManager levelManager;
    MultiplayerManager multiplayerManager;
    NetworkManager networkManager;
    SoundManager soundManager;

    int activePlayerCount;
    int enemyShootCounter;
    int networkFrameCounter;
    int nextEnemyId;
    int nextCoinId;
    int nextProjectileId;
    bool playerOneMouseShield;
    bool coopMode;
    NetworkGameMode networkGameMode;
    PlayerInputState remotePlayerInput;
    QGraphicsRectItem *finishZoneP1;
    QGraphicsRectItem *finishZoneP2;
    QGraphicsTextItem *teleportText;
    QGraphicsTextItem *networkStatusText;

    struct BubbleData
    {
        QGraphicsPixmapItem *item;
        double speedY;
        double initialX;
        double waveFrequency;
        double waveAmplitude;
        double phase;
    };

    QGraphicsPixmapItem *backgroundWhale;
    QList<BubbleData> oceanBubbles;
    void updateOceanBackground();

    struct EnemySpawnInfo
    {
        int type;
        double x;
        double y;
    };

    QList<EnemySpawnInfo> levelEnemySpawns;
    void respawnEnemies();
};

#endif
