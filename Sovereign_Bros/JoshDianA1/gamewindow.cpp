#include "gamewindow.h"

#include <QApplication>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QDebug>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QEventLoop>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QtMath>
#include <QRandomGenerator>
#include <QLinearGradient>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include "imageutils.h"

GameWindow::GameWindow(QWidget *parent)
    : QGraphicsView(parent),
      scene(new QGraphicsScene(this)),
      timer(new QTimer(this)),
      state(MainMenuState),
      finishFlag(nullptr),
      timerText(nullptr),
      scoreText(nullptr),
      controlsText(nullptr),
      levelText(nullptr),
      networkManager(this),
      soundManager(this),
      activePlayerCount(1),
      enemyShootCounter(0),
      networkFrameCounter(0),
      nextEnemyId(1),
      nextCoinId(1),
      nextProjectileId(1),
      playerOneMouseShield(false),
      coopMode(false),
      networkGameMode(OfflineGame),
      finishZoneP1(nullptr),
      finishZoneP2(nullptr),
      teleportText(nullptr),
      networkStatusText(nullptr),
      friendlyNpc(nullptr),
      friendlyTrigger(nullptr),
      shieldUnlockedForLevel2(false),
      backgroundWhale(nullptr)
{
    setupScene();
    showMainMenu();

    qApp->installEventFilter(this);

    connect(timer, &QTimer::timeout, this, &GameWindow::gameLoop);
    connect(&networkManager, &NetworkManager::connected, this, &GameWindow::handleNetworkConnected);
    connect(&networkManager, &NetworkManager::disconnected, this, &GameWindow::handleNetworkDisconnected);
    connect(&networkManager, &NetworkManager::messageReceived, this, &GameWindow::handleNetworkMessage);
    connect(&networkManager, &NetworkManager::errorOccurred, this, &GameWindow::handleNetworkError);
    connect(&networkManager, &NetworkManager::statusChanged, this, &GameWindow::updateNetworkStatus);
    timer->start(16);

    setFocus();
    viewport()->setFocus();
}

GameWindow::~GameWindow()
{
    qApp->removeEventFilter(this);
}

bool GameWindow::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (!keyEvent->isAutoRepeat())
        {
            keysPressed.insert(keyEvent->key());
        }
    }

    if (event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (!keyEvent->isAutoRepeat())
        {
            keysPressed.remove(keyEvent->key());
        }
    }

    return false;
}

void GameWindow::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        keysPressed.insert(event->key());
    }

    if (event->key() == Qt::Key_R)
    {
        if (state == PlayingState)
        {
            resetCurrentLevel();
        }
        else if (state == ResultsState)
        {
            showMainMenu();
        }
    }

    if (state == PlayingState)
    {
        if (networkGameMode == ClientGame)
        {
            if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W || event->key() == Qt::Key_Space)
            {
                soundManager.playJump();
            }

            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_L || event->key() == Qt::Key_F)
            {
                sendPlayerAction("shoot");
            }
            else if (event->key() == Qt::Key_K || event->key() == Qt::Key_G)
            {
                sendPlayerAction("grenade");
            }
            else if (event->key() == Qt::Key_T)
            {
                sendPlayerAction("teleport");
            }
        }
        else if (event->key() == Qt::Key_F)
        {
            shootPlayerBullet(0);
        }
        else if (event->key() == Qt::Key_G)
        {
            dropGrenade(0);
        }
        else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_L)
        {
            shootPlayerBullet(1);
        }
        else if (event->key() == Qt::Key_K)
        {
            dropGrenade(1);
        }
        else if (event->key() == Qt::Key_T)
        {
            if (!tryTeleportPlayer(0))
            {
                tryTeleportPlayer(1);
            }
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        keysPressed.remove(event->key());
    }

    QGraphicsView::keyReleaseEvent(event);
}

void GameWindow::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    viewport()->setFocus();

    QPointF scenePoint = mapToScene(event->pos());

    if (state == MainMenuState)
    {
        handleMenuClick(scenePoint);
        return;
    }

    if (state == ResultsState)
    {
        handleResultsClick(scenePoint);
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        if (networkGameMode == ClientGame)
        {
            sendPlayerAction("shoot");
        }
        else
        {
            shootPlayerBullet(0);
        }
    }

    if (event->button() == Qt::RightButton && canUseShield())
    {
        playerOneMouseShield = true;

        if (networkGameMode != ClientGame && !players.isEmpty())
        {
            players[0]->setShieldActive(true);
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void GameWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (state == PlayingState && event->button() == Qt::RightButton && !players.isEmpty() && networkGameMode != ClientGame)
    {
        playerOneMouseShield = false;
        players[0]->setShieldActive(false);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GameWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    setFocus();
    viewport()->setFocus();

    if (state == PlayingState && event->button() == Qt::RightButton)
    {
        if (networkGameMode == ClientGame)
        {
            sendPlayerAction("grenade");
        }
        else
        {
            dropGrenade(0);
        }
    }

    QGraphicsView::mouseDoubleClickEvent(event);
}

void GameWindow::setupScene()
{
    setScene(scene);

    // Extended scene to the left and right
    scene->setSceneRect(-400, 0, 3800, 650);

    setFixedSize(1000, 650);
    setWindowTitle("Sovereign Bros");

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setFocusPolicy(Qt::StrongFocus);
    viewport()->setMouseTracking(true);
}

void GameWindow::clearSceneData()
{
    scene->clear();

    menuButtons.clear();
    players.clear();
    playerShootCooldowns.clear();
    grenadeCooldowns.clear();
    damageCooldowns.clear();

    finishFlag = nullptr;
    finishZoneP1 = nullptr;
    finishZoneP2 = nullptr;
    timerText = nullptr;
    scoreText = nullptr;
    controlsText = nullptr;
    levelText = nullptr;
    teleportText = nullptr;
    networkStatusText = nullptr;
    backgroundWhale = nullptr;
    friendlyNpc = nullptr;
    friendlyTrigger = nullptr;
    oceanBubbles.clear();
    levelEnemySpawns.clear();

    platforms.clear();
    blockingItems.clear();
    enemies.clear();
    coins.clear();
    quicksandAreas.clear();

    obstacles.clear();
    spikes.clear();
    projectiles.clear();
    grenades.clear();
    explosions.clear();
}

void GameWindow::createQuickSand(double x, double y, double w, double h)
{
    QGraphicsRectItem *sand = scene->addRect(0, 0, w, h);
    sand->setPos(x, y);
    sand->setBrush(QBrush(QColor(170, 120, 45, 180)));
    sand->setPen(QPen(QColor(100, 70, 20), 2));
    sand->setZValue(3);

    QGraphicsTextItem *label = scene->addText("QUICKSAND");
    label->setDefaultTextColor(QColor(80, 45, 10));
    label->setFont(QFont("Arial", 9, QFont::Bold));
    label->setPos(x + 15, y - 24);
    label->setZValue(4);

    quicksandAreas.append(sand);
}

bool GameWindow::playerInQuickSand(PlayerGuy *player) const
{
    if (!player)
        return false;

    QRectF feetRect = player->sceneBoundingRect();

    // Only check the lower part of the player, so it feels like standing in sand
    feetRect.setTop(feetRect.bottom() - 22);

    for (QGraphicsRectItem *sand : quicksandAreas)
    {
        if (feetRect.intersects(sand->sceneBoundingRect()))
        {
            return true;
        }
    }

    return false;
}
void GameWindow::createSpikes(double x, double groundY, int spikeCount)
{
    const double spikeWidth = 10.0;
    const double spikeHeight = 10.0;

    for (int i = 0; i < spikeCount; i++)
    {
        QPolygonF triangle;
        triangle << QPointF(0, spikeHeight)
                 << QPointF(spikeWidth / 2.0, 0)
                 << QPointF(spikeWidth, spikeHeight);

        QGraphicsPolygonItem *spike = scene->addPolygon(triangle);
        spike->setPos(x + i * spikeWidth, groundY - spikeHeight);
        spike->setBrush(QBrush(QColor(80, 80, 80)));
        spike->setPen(QPen(Qt::black, 2));
        spike->setZValue(7);

        spikes.append(spike);
    }
}





void GameWindow::showMainMenu()
{
    state = MainMenuState;
    networkGameMode = OfflineGame;
    networkManager.disconnectFromNetwork();
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(0, 0, 1000, 650);
    scene->setBackgroundBrush(QBrush(QColor(90, 190, 150)));

    QGraphicsTextItem *title = scene->addText("        Sovereign Bros");
    title->setDefaultTextColor(Qt::black);
    title->setFont(QFont("Arial", 34, QFont::Bold));
    title->setPos(235, 120);
    title->setZValue(10);

    QGraphicsTextItem *subtitle = scene->addText("Choose a mode");
    subtitle->setDefaultTextColor(QColor(20, 70, 45));
    subtitle->setFont(QFont("Arial", 18, QFont::Bold));
    subtitle->setPos(415, 185);
    subtitle->setZValue(10);

    addMenuButton("Single Player", "single", 365, 225);
    addMenuButton("Multiplayer", "multiplayer_menu", 365, 295);
    addMenuButton("View Results", "highscores", 365, 365);
    addMenuButton("Quit", "quit", 365, 435);

    centerOn(500, 325);
}

void GameWindow::showMultiplayerMenu()
{
    state = MainMenuState;
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(0, 0, 1000, 650);
    scene->setBackgroundBrush(QBrush(QColor(75, 160, 180)));

    QGraphicsTextItem *title = scene->addText("Multiplayer");
    title->setDefaultTextColor(Qt::white);
    title->setFont(QFont("Arial", 34, QFont::Bold));
    title->setPos(385, 145);
    title->setZValue(10);

    addMenuButton("Host Game", "host", 365, 260);
    addMenuButton("Join Game", "join", 365, 340);
    addMenuButton("Back", "back", 365, 420);

    centerOn(500, 325);
}

void GameWindow::showConnectionScreen(const QString &message)
{
    state = MainMenuState;
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(0, 0, 1000, 650);
    scene->setBackgroundBrush(QBrush(QColor(50, 135, 160)));

    QGraphicsTextItem *title = scene->addText("Network Multiplayer");
    title->setDefaultTextColor(Qt::white);
    title->setFont(QFont("Arial", 32, QFont::Bold));
    title->setPos(320, 145);
    title->setZValue(10);

    networkStatusText = scene->addText(message);
    networkStatusText->setDefaultTextColor(Qt::white);
    networkStatusText->setFont(QFont("Arial", 18, QFont::Bold));
    networkStatusText->setTextWidth(760);
    networkStatusText->setPos(145, 245);
    networkStatusText->setZValue(10);

    addMenuButton("Back", "back", 365, 430);
    centerOn(500, 325);
}

void GameWindow::showHighScoresScreen()
{
    state = ResultsState;
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(0, 0, 1000, 650);
    scene->setBackgroundBrush(QBrush(QColor(35, 110, 130)));

    QGraphicsTextItem *title = scene->addText("Saved Results");
    title->setDefaultTextColor(Qt::white);
    title->setFont(QFont("Arial", 32, QFont::Bold));
    title->setPos(365, 60);
    title->setZValue(10);

    QFile file("multiplayer_results.log");
    QString results = "No saved results yet.";

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        results = QString::fromUtf8(file.readAll()).trimmed();
        if (results.isEmpty())
        {
            results = "No saved results yet.";
        }
    }

    QGraphicsTextItem *resultsText = scene->addText(results.right(2500));
    resultsText->setDefaultTextColor(Qt::white);
    resultsText->setFont(QFont("Consolas", 11, QFont::Bold));
    resultsText->setTextWidth(850);
    resultsText->setPos(80, 130);
    resultsText->setZValue(10);

    addMenuButton("Main Menu", "mainmenu", 265, 545);
    addMenuButton("Quit", "quit", 535, 545);

    centerOn(500, 325);
}

void GameWindow::addMenuButton(const QString &text, const QString &action, double x, double y)
{
    MenuButton button;
    button.rect = scene->addRect(0, 0, 270, 52);
    button.rect->setPos(x, y);
    button.rect->setBrush(QBrush(QColor(245, 245, 220)));
    button.rect->setPen(QPen(Qt::black, 2));
    button.rect->setZValue(20);
    button.action = action;

    button.label = scene->addText(text);
    button.label->setDefaultTextColor(Qt::black);
    button.label->setFont(QFont("Arial", 18, QFont::Bold));
    button.label->setPos(x + 38, y + 10);
    button.label->setZValue(21);

    menuButtons.append(button);
}

void GameWindow::handleMenuClick(const QPointF &scenePoint)
{
    for (const MenuButton &button : menuButtons)
    {
        if (button.rect->sceneBoundingRect().contains(scenePoint))
        {
            if (button.action == "single")
            {
                startGame(1);
            }
            else if (button.action == "multi")
            {
                startGame(2);
            }
            else if (button.action == "multiplayer_menu")
            {
                showMultiplayerMenu();
            }
            else if (button.action == "highscores")
            {
                showHighScoresScreen();
            }
            else if (button.action == "host")
            {
                startHostMultiplayer();
            }
            else if (button.action == "join")
            {
                joinMultiplayer();
            }
            else if (button.action == "back")
            {
                showMainMenu();
            }
            else if (button.action == "quit")
            {
                qApp->quit();
            }

            return;
        }
    }
}

void GameWindow::handleResultsClick(const QPointF &scenePoint)
{
    for (const MenuButton &button : menuButtons)
    {
        if (button.rect->sceneBoundingRect().contains(scenePoint))
        {
            if (button.action == "restart")
            {
                showMainMenu();
            }
            else if (button.action == "mainmenu")
            {
                showMainMenu();
            }
            else if (button.action == "quit")
            {
                qApp->quit();
            }

            return;
        }
    }
}

void GameWindow::startGame(int playerCount)
{
    activePlayerCount = playerCount;
    coopMode = playerCount == 2;
    scoreManager.reset(playerCount);
    levelManager.reset();
    multiplayerManager.reset();
    enemyShootCounter = 0;
    networkFrameCounter = 0;
    nextEnemyId = 1;
    nextCoinId = 1;
    nextProjectileId = 1;
    remotePlayerInput = PlayerInputState();
    playerOneMouseShield = false;

    // Shield only becomes available after touching friendly at end of level 1
    shieldUnlockedForLevel2 = false;

    loadLevel(1);
}

void GameWindow::startHostMultiplayer()
{
    bool ok = false;
    int port = QInputDialog::getInt(this, "Host Multiplayer", "Port:", 45454, 1024, 65535, 1, &ok);

    if (!ok)
        return;

    networkGameMode = HostGame;

    if (networkManager.startServer(static_cast<quint16>(port)))
    {
        showConnectionScreen(QString("Hosting on port %1.\nAdvertising this game on the local network for Player 2 discovery.").arg(port));
    }
}

void GameWindow::joinMultiplayer()
{
    bool ok = false;
    networkManager.startDiscovery();
    showConnectionScreen("Searching for multiplayer hosts on this network...");

    QEventLoop discoveryWait;
    QTimer::singleShot(2500, &discoveryWait, &QEventLoop::quit);
    discoveryWait.exec();

    QList<NetworkManager::DiscoveredHost> discoveredHosts = networkManager.discoveredHosts();
    QString host;
    quint16 port = 45454;

    if (!discoveredHosts.isEmpty())
    {
        QStringList hostOptions;
        for (const NetworkManager::DiscoveredHost &discoveredHost : discoveredHosts)
        {
            hostOptions << QString("%1 (%2:%3)")
                           .arg(discoveredHost.name)
                           .arg(discoveredHost.address)
                           .arg(discoveredHost.port);
        }
        hostOptions << "Manual IP address...";

        QString selectedHost = QInputDialog::getItem(this,
                                                     "Join Multiplayer",
                                                     "Discovered hosts:",
                                                     hostOptions,
                                                     0,
                                                     false,
                                                     &ok);

        if (!ok)
        {
            showMultiplayerMenu();
            return;
        }

        int selectedIndex = hostOptions.indexOf(selectedHost);
        if (selectedIndex >= 0 && selectedIndex < discoveredHosts.size())
        {
            host = discoveredHosts[selectedIndex].address;
            port = discoveredHosts[selectedIndex].port;
        }
    }

    if (host.isEmpty())
    {
        host = QInputDialog::getText(this, "Join Multiplayer", "Host IP address:", QLineEdit::Normal, "127.0.0.1", &ok);

        if (!ok || host.trimmed().isEmpty())
        {
            showMultiplayerMenu();
            return;
        }

        int selectedPort = QInputDialog::getInt(this, "Join Multiplayer", "Port:", 45454, 1024, 65535, 1, &ok);

        if (!ok)
        {
            showMultiplayerMenu();
            return;
        }

        port = static_cast<quint16>(selectedPort);
    }

    networkGameMode = ClientGame;
    networkManager.connectToServer(host.trimmed(), port);
    showConnectionScreen(QString("Connecting to %1:%2...").arg(host.trimmed()).arg(port));
}

void GameWindow::loadLevel(int levelNumber)
{
    state = PlayingState;
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(-400, 0, 3800, 650);

    levelManager.setCurrentLevel(levelNumber);
    levelManager.startLevelTimer();

    if (coopMode)
    {
        if (levelNumber == 1)
        {
            loadMultiplayerCoopLevel();
        }
        else
        {
            loadMultiplayerCoopOceanLevel();
        }
    }
    else if (levelNumber == 1)
    {
        loadJungleLevel();
    }
    else
    {
        loadOceanLevel();
    }

    setupPlayers(activePlayerCount);

    timerText = scene->addText("");
    timerText->setDefaultTextColor(Qt::black);
    timerText->setFont(QFont("Arial", 17, QFont::Bold));
    timerText->setZValue(100);

    scoreText = scene->addText("");
    scoreText->setDefaultTextColor(Qt::black);
    scoreText->setFont(QFont("Arial", 17, QFont::Bold));
    scoreText->setZValue(100);

    levelText = scene->addText("");
    levelText->setDefaultTextColor(Qt::black);
    levelText->setFont(QFont("Arial", 17, QFont::Bold));
    levelText->setZValue(100);

    QString controls = "P1: A/D move, W/Space jump, F or Left Click shoot, G or double Right Click grenade, hold Right Click shield, R reset";

    if (activePlayerCount == 2)
    {
        if (networkGameMode == ClientGame)
        {
          controls = "You are Player 2: Left/Right move, Up jump, Enter or L shoot, K grenade, hold Right Click shield";
        }
        else if (networkGameMode == HostGame)
        {
            controls += "\nNetwork game: Player 2 controls come from the client. T teleports the ready player to their partner";
        }
        else
        {
            controls += "\nP2: Left/Right move, Up jump, Enter or L shoot, K grenade, O shield | T teleports the ready player to their partner";
        }
    }

    controlsText = scene->addText(controls);
    controlsText->setDefaultTextColor(Qt::black);
    controlsText->setFont(QFont("Arial", 10, QFont::Bold));
    controlsText->setZValue(100);

    teleportText = scene->addText("");
    teleportText->setDefaultTextColor(QColor(255, 255, 0));
    teleportText->setFont(QFont("Arial", 18, QFont::Bold));
    teleportText->setZValue(200);

    networkStatusText = scene->addText(networkManager.statusText());
    networkStatusText->setDefaultTextColor(QColor(20, 40, 120));
    networkStatusText->setFont(QFont("Arial", 11, QFont::Bold));
    networkStatusText->setZValue(100);

    updateHud();
    updateCamera();

    setFocus();
    viewport()->setFocus();
}

void GameWindow::createFriendly(double x, double groundY)
{
    QImage img(":/GUI/Friendly.png");
    img = cropTransparent(img);

    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(95, 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    friendlyNpc = scene->addPixmap(pix);
    friendlyNpc->setPos(x, groundY - 120);
    friendlyNpc->setZValue(6);

    friendlyTrigger = scene->addRect(0, 0, 105, 120);
    friendlyTrigger->setPos(x, groundY - 120);
    friendlyTrigger->setBrush(Qt::transparent);
    friendlyTrigger->setPen(Qt::NoPen);
    friendlyTrigger->setZValue(5);

    QGraphicsTextItem *label = scene->addText("FRIENDLY");
    label->setDefaultTextColor(Qt::black);
    label->setFont(QFont("Arial", 10, QFont::Bold));
    label->setPos(x - 5, groundY - 145);
    label->setZValue(7);
}

bool GameWindow::canUseShield() const
{
    // In multiplayer, reaching level 2 means both players already completed level 1.
    // So both players must be allowed to use shields in level 2.
    if (coopMode)
    {
        return levelManager.currentLevel() >= 2;
    }

    // In single player, keep the friendly unlock rule.
    return shieldUnlockedForLevel2 && levelManager.currentLevel() >= 2;
}

void GameWindow::loadMultiplayerCoopOceanLevel()
{
    scene->setBackgroundBrush(QBrush(QColor(35, 140, 205)));

    QImage whaleImg(":/GUI/Background whale.png");
    if (!whaleImg.isNull())
    {
        whaleImg = cropTransparent(whaleImg);
        whaleImg = whaleImg.mirrored(true, false);
        QPixmap whalePix = QPixmap::fromImage(whaleImg);
        whalePix = whalePix.scaled(260, 130, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        backgroundWhale = scene->addPixmap(whalePix);
        backgroundWhale->setPos(-350, 150);
        backgroundWhale->setZValue(0.1);
    }

    // Extended ocean floor for multiplayer spawn area
    addPlatform(-400, 580, 3400, 70, QColor(20, 100, 150));

    // Same ocean platforms as single player
    addPlatform(220, 500, 230, 28, QColor(100, 210, 210));
    addPlatform(600, 455, 260, 28, QColor(100, 210, 210));
    addPlatform(980, 520, 240, 28, QColor(100, 210, 210));
    addPlatform(1360, 430, 260, 28, QColor(100, 210, 210));
    addPlatform(1780, 505, 260, 28, QColor(100, 210, 210));
    addPlatform(2200, 440, 250, 28, QColor(100, 210, 210));
    addPlatform(2540, 505, 210, 28, QColor(100, 210, 210));

    // Extra higher multiplayer platforms
    addPlatform(460, 310, 150, 28, QColor(80, 190, 210));
    addPlatform(850, 285, 150, 28, QColor(80, 190, 210));
    addPlatform(1220, 300, 170, 28, QColor(80, 190, 210));
    addPlatform(1660, 260, 170, 28, QColor(80, 190, 210));
    addPlatform(2060, 295, 180, 28, QColor(80, 190, 210));
    addPlatform(2380, 260, 170, 28, QColor(80, 190, 210));
    addPlatform(2720, 320, 160, 28, QColor(80, 190, 210));

    // Coins from single player ocean
    createCoin(280, 460);
    createCoin(690, 415);
    createCoin(1050, 480);
    createCoin(1430, 390);
    createCoin(1850, 465);
    createCoin(2290, 400);
    createCoin(2600, 465);
    createCoin(2860, 540);

    // Extra coins for the higher multiplayer route
    createCoin(500, 270);
    createCoin(900, 245);
    createCoin(1270, 260);
    createCoin(1710, 220);
    createCoin(2120, 255);
    createCoin(2430, 220);
    createCoin(2770, 280);

    createSeaweed(360, 580);
    createSeaweed(840, 580);
    createSeaweed(1280, 580);
    createSeaweed(1960, 580);
    createSeaweed(2440, 580);

    // Ocean enemies from single player
    addEnemy(new SharkEnemy(), 520, 505);
    addEnemy(new SharkEnemy(), 1120, 430);
    addEnemy(new SharkEnemy(), 1900, 450);
    addEnemy(new SharkEnemy(), 2460, 380);

    addEnemy(new OctopusEnemy(), 760, 498);
    addEnemy(new OctopusEnemy(), 1500, 353);
    addEnemy(new OctopusEnemy(), 2280, 363);

    // Extra multiplayer enemies on higher path
    addEnemy(new OctopusEnemy(), 930, 205);
    addEnemy(new SharkEnemy(), 1700, 210);
    addEnemy(new OctopusEnemy(), 2420, 180);

    // Two-player finish area
    finishFlag = scene->addRect(0, 0, 130, 115);
    finishFlag->setPos(2850, 465);
    finishFlag->setBrush(QBrush(QColor(255, 220, 60)));
    finishFlag->setPen(QPen(Qt::black, 2));
    finishFlag->setZValue(4);

    finishZoneP1 = scene->addRect(0, 0, 58, 95);
    finishZoneP1->setPos(2862, 480);
    finishZoneP1->setBrush(QBrush(QColor(0, 80, 255, 95)));
    finishZoneP1->setPen(QPen(Qt::blue, 2));
    finishZoneP1->setZValue(5);

    finishZoneP2 = scene->addRect(0, 0, 58, 95);
    finishZoneP2->setPos(2922, 480);
    finishZoneP2->setBrush(QBrush(QColor(255, 145, 0, 110)));
    finishZoneP2->setPen(QPen(QColor(180, 90, 0), 2));
    finishZoneP2->setZValue(5);

    QGraphicsTextItem *exitText = scene->addText("BOTH PLAYERS");
    exitText->setDefaultTextColor(Qt::black);
    exitText->setFont(QFont("Arial", 10, QFont::Bold));
    exitText->setPos(2856, 438);
    exitText->setZValue(6);
}



QGraphicsRectItem* GameWindow::addPlatform(double x, double y, double w, double h, const QColor &color)
{
    QGraphicsRectItem *platform = scene->addRect(0, 0, w, h);
    platform->setPos(x, y);
    platform->setBrush(Qt::transparent);
    platform->setPen(Qt::NoPen);

    QGraphicsRectItem *visual = new QGraphicsRectItem(0, 0, w, h, platform);
    QLinearGradient gradient(0, 0, 0, h);
    gradient.setColorAt(0, color.lighter(120));
    gradient.setColorAt(1, color.darker(150));
    visual->setBrush(QBrush(gradient));

    QPen pen(color.darker(180));
    pen.setWidth(2);
    visual->setPen(pen);

    QGraphicsRectItem *highlight = new QGraphicsRectItem(0, 0, w, qMin(8.0, h), platform);
    highlight->setBrush(QBrush(color.lighter(150)));
    highlight->setPen(Qt::NoPen);

    platform->setZValue(1);
    platforms.append(platform);
    return platform;
}

void GameWindow::loadJungleLevel()
{
    scene->setBackgroundBrush(QBrush(QColor(92, 210, 210)));

    // Extended ground to the right for the friendly ending
    addPlatform(0, 580, 3400, 70, Qt::darkGreen);

    addPlatform(250, 500, 200, 30, QColor(100, 90, 70));
    addPlatform(600, 360, 200, 30, QColor(100, 90, 70));
    //addPlatform(950, 520, 250, 30, QColor(100, 90, 70));
    addPlatform(1350, 450, 220, 30, QColor(100, 90, 70));
    addPlatform(1750, 380, 250, 30, QColor(100, 90, 70));
    addPlatform(2200, 500, 250, 30, QColor(100, 90, 70));
    addPlatform(2500, 420, 200, 30, QColor(100, 90, 70));


    // Five flying fish that bounce around and chase the player
    addEnemy(new FlyingFishEnemy(), 520, 170);
    addEnemy(new FlyingFishEnemy(), 1050, 210);
    addEnemy(new FlyingFishEnemy(), 1550, 160);
    addEnemy(new FlyingFishEnemy(), 2100, 220);
    addEnemy(new FlyingFishEnemy(), 2650, 180);

    createCoin(310, 460);
    createCoin(660, 320);
    createCoin(1020, 480);
    createCoin(1410, 410);
    createCoin(1810, 340);
    createCoin(2300, 460);
    createCoin(2550, 380);
    createCoin(2860, 540);

    Obstacle *bouncePad1 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad1->setPos(500, 560);
    scene->addItem(bouncePad1);
    obstacles.append(bouncePad1);

    Obstacle *bouncePad2 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad2->setPos(1600, 560);
    scene->addItem(bouncePad2);
    obstacles.append(bouncePad2);

    Obstacle *bouncePad3 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad3->setPos(970, 560);
    scene->addItem(bouncePad3);
    obstacles.append(bouncePad3);

    createTree(370, 580, false, false);
    createTree(820, 580, false, false);
    createTree(1280, 580, true, false);
    createTree(1900, 580, false, false);
    createTree(2360, 580, false, false);
    createTree(2750, 580, false, false);

    createCloud(180, 70);
    createCloud(780, 60);
    createCloud(1450, 80);
    createCloud(2050, 65);
    createCloud(2600, 75);

    addEnemy(new FishEnemy(), 750, 525);
    addEnemy(new FishEnemy(), 1040, 525);
    addEnemy(new FishEnemy(), 2220, 445);
    addEnemy(new BigEnemy(), 1500, 485);
    addEnemy(new BirdEnemy(), 450, 330);
    addEnemy(new BirdEnemy(), 850, 320);
    addEnemy(new BirdEnemy(), 1700, 310);
    addEnemy(new BirdEnemy(), 2400, 310);
    addEnemy(new BirdEnemy(), 2580, 290);
    addEnemy(new BirdEnemy(), 2720, 320);
    addEnemy(new PlantEnemy(), 720, 500);
    addEnemy(new PlantEnemy(), 1180, 500);
    addEnemy(new PlantEnemy(), 1880, 500);
    addEnemy(new PlantEnemy(), 2320, 500);

    // No exit door in level 1 anymore
    // Spikes before Friendly, so the player must jump over them
    createSpikes(3010, 580, 1);

    // No exit door in level 1 anymore
    createFriendly(3170, 580);
}

void GameWindow::loadOceanLevel()
{
    scene->setBackgroundBrush(QBrush(QColor(35, 140, 205)));

    QImage whaleImg(":/GUI/Background whale.png");
    if (!whaleImg.isNull())
    {
        whaleImg = cropTransparent(whaleImg);
        whaleImg = whaleImg.mirrored(true, false);
        QPixmap whalePix = QPixmap::fromImage(whaleImg);
        whalePix = whalePix.scaled(260, 130, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        backgroundWhale = scene->addPixmap(whalePix);
        backgroundWhale->setPos(-350, 150);
        backgroundWhale->setZValue(0.1);
    }

   addPlatform(0, 580, 3400, 70, QColor(20, 100, 150));
   addPlatform(220, 500, 230, 28, QColor(100, 210, 210));
   addPlatform(600, 455, 260, 28, QColor(100, 210, 210));
   addPlatform(980, 520, 240, 28, QColor(100, 210, 210));
   addPlatform(1360, 430, 260, 28, QColor(100, 210, 210));
   addPlatform(1780, 505, 260, 28, QColor(100, 210, 210));
   addPlatform(2200, 440, 250, 28, QColor(100, 210, 210));
   addPlatform(2540, 505, 210, 28, QColor(100, 210, 210));



    createCoin(280, 460);
    createCoin(690, 415);
    createCoin(1050, 480);
    createCoin(1430, 390);
    createCoin(1850, 465);
    createCoin(2290, 400);
    createCoin(2600, 465);
    createCoin(2860, 540);

    createSeaweed(360, 580);
    createSeaweed(840, 580);
    createSeaweed(1280, 580);
    createSeaweed(1960, 580);
    createSeaweed(2440, 580);

    addEnemy(new SharkEnemy(), 520, 505);
    addEnemy(new SharkEnemy(), 1120, 430);
    addEnemy(new SharkEnemy(), 1900, 450);
    addEnemy(new SharkEnemy(), 2460, 380);
    addEnemy(new OctopusEnemy(), 760, 498);
    addEnemy(new OctopusEnemy(), 1500, 353);
    addEnemy(new OctopusEnemy(), 2280, 363);

    // Extra final platform before the quicksand area
   // addPlatform(2780, 505, 180, 28, QColor(100, 210, 210));

    // Coins near the extended ending
    createCoin(2820, 465);
    createCoin(2960, 540);
    createCoin(3090, 540);

    // Longer quicksand patch before the Level 2 door
    createQuickSand(2920, 560, 210, 20);

    // Door after the quicksand
    createExit(3220, 480, QColor(255, 220, 60));


}

void GameWindow::loadMultiplayerCoopLevel()
{
    scene->setBackgroundBrush(QBrush(QColor(92, 210, 210)));

    // Extended floor left and right
    addPlatform(-400, 580, 3800, 70, Qt::darkGreen);

    // Same main jungle platforms as single player
    addPlatform(250, 500, 200, 30, QColor(100, 90, 70));
    addPlatform(600, 360, 200, 30, QColor(100, 90, 70));
    addPlatform(950, 520, 250, 30, QColor(100, 90, 70));
    addPlatform(1350, 450, 220, 30, QColor(100, 90, 70));
    addPlatform(1750, 380, 250, 30, QColor(100, 90, 70));
    addPlatform(2200, 500, 250, 30, QColor(100, 90, 70));
    addPlatform(2500, 420, 200, 30, QColor(100, 90, 70));

    addEnemy(new FlyingFishEnemy(), 520, 170);
    addEnemy(new FlyingFishEnemy(), 1050, 210);
    addEnemy(new FlyingFishEnemy(), 1550, 160);
    addEnemy(new FlyingFishEnemy(), 2100, 220);
    addEnemy(new FlyingFishEnemy(), 2650, 180);

    // Extra higher multiplayer platforms
    addPlatform(480, 270, 150, 28, QColor(95, 85, 65));
    addPlatform(850, 250, 150, 28, QColor(95, 85, 65));
    addPlatform(1180, 300, 170, 28, QColor(95, 85, 65));
    addPlatform(1580, 240, 170, 28, QColor(95, 85, 65));
    addPlatform(1980, 280, 180, 28, QColor(95, 85, 65));
    addPlatform(2320, 250, 170, 28, QColor(95, 85, 65));
    addPlatform(2700, 300, 160, 28, QColor(95, 85, 65));

    createCoin(310, 460);
    createCoin(660, 320);
    createCoin(1020, 480);
    createCoin(1410, 410);
    createCoin(1810, 340);
    createCoin(2300, 460);
    createCoin(2550, 380);
    createCoin(2860, 540);

    createCoin(520, 230);
    createCoin(900, 210);
    createCoin(1230, 260);
    createCoin(1630, 200);
    createCoin(2040, 240);
    createCoin(2370, 210);
    createCoin(2750, 260);

    Obstacle *bouncePad1 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad1->setPos(500, 560);
    scene->addItem(bouncePad1);
    obstacles.append(bouncePad1);

    Obstacle *bouncePad2 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad2->setPos(1600, 560);
    scene->addItem(bouncePad2);
    obstacles.append(bouncePad2);

    Obstacle *bouncePad3 = new Obstacle(Obstacle::BouncePad, 100, 20);
    bouncePad3->setPos(2420, 560);
    scene->addItem(bouncePad3);
    obstacles.append(bouncePad3);

    createTree(370, 580, false, true);
    createTree(820, 580, false, true);
    createTree(1280, 580, true, false);
    createTree(1900, 580, false, true);
    createTree(2360, 580, false, true);
    createTree(2750, 580, false, true);

    createCloud(180, 70);
    createCloud(780, 60);
    createCloud(1450, 80);
    createCloud(2050, 65);
    createCloud(2600, 75);

    addEnemy(new FishEnemy(), 750, 525);
    addEnemy(new FishEnemy(), 1040, 525);
    addEnemy(new FishEnemy(), 2220, 445);
    addEnemy(new BigEnemy(), 1500, 485);

    addEnemy(new BirdEnemy(), 450, 330);
    addEnemy(new BirdEnemy(), 850, 320);
    addEnemy(new BirdEnemy(), 1700, 310);
    addEnemy(new BirdEnemy(), 2400, 310);
    addEnemy(new BirdEnemy(), 2580, 290);
    addEnemy(new BirdEnemy(), 2720, 320);

    addEnemy(new PlantEnemy(), 720, 500);
    addEnemy(new PlantEnemy(), 1180, 500);
    addEnemy(new PlantEnemy(), 1880, 500);
    addEnemy(new PlantEnemy(), 2320, 500);

    addEnemy(new BirdEnemy(), 600, 210);
    addEnemy(new BirdEnemy(), 1300, 240);
    addEnemy(new BirdEnemy(), 2100, 220);

    // Friendly instead of finish door in level 1
    // Spikes before Friendly, so both players must jump over them
    createSpikes(3010, 580, 1);

    // Friendly instead of finish door in level 1
    createFriendly(3170, 580);
}
void GameWindow::setupPlayers(int playerCount)
{
    PlayerGuy *player1 = new PlayerGuy(1, levelManager.currentLevel());

    if (coopMode)
    {
        // Multiplayer spawn area, on the extended left floor
        player1->resetPosition(QPointF(-340, 472));
    }
    else
    {
        // Single player must stay at the normal start position
        player1->resetPosition(QPointF(50, 472));
    }

    scene->addItem(player1);
    players.append(player1);
    playerShootCooldowns.append(0);
    grenadeCooldowns.append(0);
    damageCooldowns.append(0);

    if (playerCount == 2)
    {
        PlayerGuy *player2 = new PlayerGuy(2, levelManager.currentLevel());

        // Player 2 only exists in multiplayer
        player2->resetPosition(QPointF(-220, 472));

        scene->addItem(player2);
        players.append(player2);
        playerShootCooldowns.append(0);
        grenadeCooldowns.append(0);
        damageCooldowns.append(0);
    }
}

void GameWindow::createTree(double x, double y, bool isPlatform, bool isFullSolid)
{
    QImage img(":/GUI/Tree.png");
    img = cropTransparent(img);
    QPixmap treePix = QPixmap::fromImage(img);

    // Deterministic scale based on X coordinate (keeps network host & client in sync)
    int intX = qRound(x);
    double scale = 0.82 + ((intX % 37) / 100.0); // range: 0.82 to 1.18
    double w = 130.0 * scale;
    double h = 155.0 * scale;

    treePix = treePix.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QGraphicsPixmapItem *treeVisual = scene->addPixmap(treePix);
    treeVisual->setPos(x - w / 2.0, y - h);
    treeVisual->setZValue(0); // Always render behind platforms (which have ZValue = 1)

    if (isFullSolid)
    {
        QGraphicsRectItem *treeHitBox = scene->addRect(0, 0, w, h);
        treeHitBox->setPos(x - w / 2.0, y - h);
        treeHitBox->setBrush(Qt::transparent);
        treeHitBox->setPen(Qt::NoPen);
        treeHitBox->setZValue(-1);
        platforms.append(treeHitBox);
        blockingItems.append(treeHitBox);
    }
    else if (isPlatform)
    {
        QGraphicsRectItem *treeHitBox = scene->addRect(0, 0, w, 20); // Platform at the top of the tree
        treeHitBox->setPos(x - w / 2.0, y - h);
        treeHitBox->setBrush(Qt::transparent);
        treeHitBox->setPen(Qt::NoPen);
        treeHitBox->setZValue(-1);
        platforms.append(treeHitBox);
        blockingItems.append(treeHitBox);
    }
}

void GameWindow::createCloud(double x, double y)
{
    QImage img(":/GUI/cloud.png");
    img = cropTransparent(img);
    QPixmap cloudPix = QPixmap::fromImage(img);
    cloudPix = cloudPix.scaled(145, 45, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QGraphicsPixmapItem *cloudVisual = scene->addPixmap(cloudPix);
    cloudVisual->setPos(x, y - 10);
    cloudVisual->setZValue(1);
}

void GameWindow::createSeaweed(double x, double y)
{
    QImage img(":/GUI/seeweed.png");
    img = cropTransparent(img);
    QPixmap seaweedPix = QPixmap::fromImage(img);
    seaweedPix = seaweedPix.scaled(60, 95, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QGraphicsPixmapItem *seaweedVisual = scene->addPixmap(seaweedPix);
    seaweedVisual->setPos(x - 12, y - 95);
    seaweedVisual->setZValue(2);
}

void GameWindow::createExit(double x, double y, const QColor &color)
{
    finishFlag = scene->addRect(0, 0, 48, 100);
    finishFlag->setPos(x, y);
    finishFlag->setBrush(QBrush(color));
    finishFlag->setPen(QPen(Qt::black, 2));
    finishFlag->setZValue(4);

    QGraphicsTextItem *exitText = scene->addText("EXIT");
    exitText->setDefaultTextColor(Qt::black);
    exitText->setFont(QFont("Arial", 10, QFont::Bold));
    exitText->setPos(x + 5, y - 24);
    exitText->setZValue(5);
}

void GameWindow::createCoin(double x, double y)
{
    QGraphicsRectItem *coin = scene->addRect(0, 0, 25, 25);
    coin->setPos(x, y);
    coin->setBrush(Qt::transparent);
    coin->setPen(Qt::NoPen);
    coin->setZValue(5);
    coin->setData(9, nextCoinId++);

    QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, 25, 25, coin);
    QRadialGradient grad(12.5, 12.5, 12.5);
    grad.setColorAt(0, QColor(255, 255, 150));
    grad.setColorAt(0.7, QColor(255, 210, 0));
    grad.setColorAt(1, QColor(200, 140, 0));
    visual->setBrush(QBrush(grad));
    visual->setPen(QPen(QColor(180, 120, 0), 2));

    QGraphicsEllipseItem *inner = new QGraphicsEllipseItem(5, 5, 15, 15, coin);
    inner->setBrush(Qt::transparent);
    inner->setPen(QPen(QColor(255, 230, 50), 2));

    coins.append(coin);
}

void GameWindow::addEnemy(BaseEnemy *enemy, double x, double y)
{
    enemy->setPos(x, y);
    enemy->setData(9, nextEnemyId++);
    scene->addItem(enemy);
    enemies.append(enemy);

    EnemySpawnInfo spawn;
    spawn.type = enemy->getKind();
    spawn.x = x;
    spawn.y = y;
    levelEnemySpawns.append(spawn);
}

void GameWindow::gameLoop()
{
    if (state != PlayingState)
        return;

    updateOceanBackground();

    if (networkGameMode == ClientGame)
    {
        sendClientInput();
        updateHud();
        updateCamera();
        return;
    }

    levelManager.tick();

    for (int i = 0; i < playerShootCooldowns.size(); i++)
    {
        if (playerShootCooldowns[i] > 0)
        {
            playerShootCooldowns[i]--;
        }
    }

    for (int i = 0; i < grenadeCooldowns.size(); i++)
    {
        if (grenadeCooldowns[i] > 0)
        {
            grenadeCooldowns[i]--;
        }
    }

    for (int i = 0; i < damageCooldowns.size(); i++)
    {
        if (damageCooldowns[i] > 0)
        {
            damageCooldowns[i]--;
        }
    }

    updatePlayers();
    multiplayerManager.tickTeleportTimers(players);
    updateCoopPlayerCollision();
    updateEnemies();
    updateProjectiles();
    updateGrenades();
    updateExplosions();
    checkCollisions();
    updateHud();
    updateCamera();

    if (networkGameMode == HostGame)
    {
        networkFrameCounter++;
        if (networkFrameCounter % 2 == 0)
        {
            broadcastGameState();
        }
    }
}

void GameWindow::updatePlayers()
{
    if (players.isEmpty())
        return;

    PlayerGuy *player1 = players[0];
    player1->stopMoving();

    bool p1InQuickSand = playerInQuickSand(player1);

    if (keysPressed.contains(Qt::Key_A))
    {
        if (p1InQuickSand)
            player1->moveLeftSlow();
        else
            player1->moveLeft();
    }

    if (keysPressed.contains(Qt::Key_D))
    {
        if (p1InQuickSand)
            player1->moveRightSlow();
        else
            player1->moveRight();
    }

    if (keysPressed.contains(Qt::Key_Space) || keysPressed.contains(Qt::Key_W))
    {
        if (player1->jump())
        {
            soundManager.playJump();
            broadcastSoundEvent("jump", 1);
        }
    }

    // Shield only works in level 2 after being unlocked
    player1->setShieldActive(canUseShield() && playerOneMouseShield);
    player1->updatePhysics(platforms);

    if (player1->y() > 800)
    {
        resetPlayerToSpawn(0);
    }

    if (players.size() > 1)
    {
        if (networkGameMode == HostGame)
        {
            applyInputToPlayer(1, remotePlayerInput);
        }
        else
        {
            PlayerGuy *player2 = players[1];
            player2->stopMoving();

            bool p2InQuickSand = playerInQuickSand(player2);

            if (keysPressed.contains(Qt::Key_Left))
            {
                if (p2InQuickSand)
                    player2->moveLeftSlow();
                else
                    player2->moveLeft();
            }

            if (keysPressed.contains(Qt::Key_Right))
            {
                if (p2InQuickSand)
                    player2->moveRightSlow();
                else
                    player2->moveRight();
            }

            if (keysPressed.contains(Qt::Key_Up))
            {
                if (player2->jump())
                {
                    soundManager.playJump();
                }
            }

            // Shield only works in level 2 after being unlocked
            player2->setShieldActive(canUseShield() && keysPressed.contains(Qt::Key_O));
            player2->updatePhysics(platforms);
        }

        if (players[1]->y() > 800)
        {
            resetPlayerToSpawn(1);
        }
    }
}

void GameWindow::applyInputToPlayer(int playerIndex, const PlayerInputState &input)
{
    if (playerIndex < 0 || playerIndex >= players.size())
        return;

    PlayerGuy *player = players[playerIndex];
    player->stopMoving();

    bool inQuickSand = playerInQuickSand(player);

    if (input.left)
    {
        if (inQuickSand)
            player->moveLeftSlow();
        else
            player->moveLeft();
    }

    if (input.right)
    {
        if (inQuickSand)
            player->moveRightSlow();
        else
            player->moveRight();
    }

    if (input.jump)
    {
        if (player->jump())
        {
            soundManager.playJump();
            broadcastSoundEvent("jump", playerIndex + 1);
        }
    }

    // Shield only works in level 2 after being unlocked
    player->setShieldActive(canUseShield() && input.shield);
    player->updatePhysics(platforms);
}
void GameWindow::updateCoopPlayerCollision()
{
    if (!coopMode || players.size() < 2)
        return;

    bool bounced = false;

    if (multiplayerManager.tryHeadBounce(players[0], 0, players[1], 1))
    {
        scoreManager.addBonus(1, MultiplayerManager::HeadBounceAssistPoints);
        soundManager.playBounce();
        broadcastSoundEvent("bounce", 1);
        bounced = true;
    }
    else if (multiplayerManager.tryHeadBounce(players[1], 1, players[0], 0))
    {
        scoreManager.addBonus(0, MultiplayerManager::HeadBounceAssistPoints);
        soundManager.playBounce();
        broadcastSoundEvent("bounce", 2);
        bounced = true;
    }

    if (!bounced)
    {
        multiplayerManager.resolveSideCollision(players[0], players[1]);
    }
}

void GameWindow::handleNetworkConnected()
{
    if (networkGameMode == HostGame)
    {
        startGame(2);
        qDebug() << "Player assigned: host is Player 1, client is Player 2";
        qDebug() << "Game started by host";
        QJsonObject startMessage;
        startMessage["type"] = "startGame";
        startMessage["level"] = 1;
        networkManager.broadcastMessage(startMessage);
        broadcastGameState();
    }
    else if (networkGameMode == ClientGame)
    {
        qDebug() << "Connected as client. Waiting for player assignment.";
        updateNetworkStatus("Connected. Waiting for host to start the game...");
    }
}

void GameWindow::handleNetworkDisconnected()
{
    updateNetworkStatus(networkManager.statusText());
}

void GameWindow::handleNetworkError(const QString &error)
{
    updateNetworkStatus(error);
    if (state == MainMenuState)
    {
        QMessageBox::warning(this, "Network Error", error);
    }
}

void GameWindow::updateNetworkStatus(const QString &status)
{
    if (networkStatusText)
    {
        networkStatusText->setPlainText(status);
    }
}

void GameWindow::handleNetworkMessage(const QJsonObject &message)
{
    QString type = message["type"].toString();

    if (networkGameMode == HostGame)
    {
        if (type == "playerInput")
        {
            remotePlayerInput.left = message["left"].toBool();
            remotePlayerInput.right = message["right"].toBool();
            remotePlayerInput.jump = message["jump"].toBool();
            remotePlayerInput.shield = message["shield"].toBool();
        }
        else if (type == "shoot")
        {
            shootPlayerBullet(1);
        }
        else if (type == "playerAction")
        {
            handlePlayerAction(message);
        }
    }
    else if (networkGameMode == ClientGame)
    {
        if (type == "gameState")
        {
            applyGameState(message);
        }

        else if (type == "startGame")
        {
            qDebug() << "Player assigned: client is Player 2";
            qDebug() << "Game started by host message";

            int startLevel = message["level"].toInt(1);

            activePlayerCount = 2;
            coopMode = true;
            scoreManager.reset(2);
            levelManager.reset();
            multiplayerManager.reset();
            enemyShootCounter = 0;
            networkFrameCounter = 0;
            nextEnemyId = 1;
            nextCoinId = 1;
            nextProjectileId = 1;
            remotePlayerInput = PlayerInputState();
            playerOneMouseShield = false;

            loadLevel(startLevel);
        }

        else if (type == "levelChanged")
        {
            int level = message["level"].toInt(1);
            if (levelManager.currentLevel() != level)
            {
                loadLevel(level);
            }
        }
        else if (type == "gameOver")
        {
            applyGameState(message);
            showResultsScreen();
        }
        else if (type == "coinCollected")
        {
            int coinId = message["coinId"].toInt();
            for (QGraphicsRectItem *coin : coins)
            {
                if (coin->data(9).toInt() == coinId)
                {
                    coin->hide();
                    break;
                }
            }
        }
        else if (type == "enemyKilled")
        {
            int enemyId = message["enemyId"].toInt();
            for (BaseEnemy *enemy : enemies)
            {
                if (enemy->data(9).toInt() == enemyId)
                {
                    enemy->hide();
                    break;
                }
            }
        }
        else if (type == "soundEvent")
        {
            QString soundName = message["sound"].toString();
            int sourcePlayer = message["sourcePlayer"].toInt(0);
            if (!(sourcePlayer == 2 && soundName == "jump"))
            {
                playSoundEvent(soundName);
            }
        }
    }
}

void GameWindow::handlePlayerAction(const QJsonObject &message)
{
    QString action = message["action"].toString();

    if (action == "shoot")
    {
        shootPlayerBullet(1);
    }
    else if (action == "grenade")
    {
        dropGrenade(1);
    }
    else if (action == "teleport")
    {
        tryTeleportPlayer(1);
    }
}

QJsonObject GameWindow::buildInputMessage() const
{
    QJsonObject message;
    message["type"] = "playerInput";
    message["left"] = keysPressed.contains(Qt::Key_Left) || keysPressed.contains(Qt::Key_A);
    message["right"] = keysPressed.contains(Qt::Key_Right) || keysPressed.contains(Qt::Key_D);
    message["jump"] = keysPressed.contains(Qt::Key_Up) || keysPressed.contains(Qt::Key_W) || keysPressed.contains(Qt::Key_Space);
    message["shield"] = playerOneMouseShield;
    return message;
}

void GameWindow::sendClientInput()
{
    if (networkGameMode == ClientGame && networkManager.isConnected())
    {
        networkManager.sendMessage(buildInputMessage());
    }
}

void GameWindow::sendPlayerAction(const QString &action)
{
    if (networkGameMode != ClientGame || !networkManager.isConnected())
        return;

    QJsonObject message;
    message["action"] = action;

    if (action == "shoot")
    {
        message["type"] = "shoot";
    }
    else
    {
        message["type"] = "playerAction";
    }

    networkManager.sendMessage(message);
}

void GameWindow::playSoundEvent(const QString &soundName)
{
    if (soundName == "shoot")
    {
        soundManager.playShoot();
    }
    else if (soundName == "explosion")
    {
        soundManager.playExplosion();
    }
    else if (soundName == "jump")
    {
        soundManager.playJump();
    }
    else if (soundName == "bounce")
    {
        soundManager.playBounce();
    }
    else if (soundName == "coin")
    {
        soundManager.playCoin();
    }
    else if (soundName == "levelComplete")
    {
        soundManager.playLevelComplete();
    }
}

void GameWindow::broadcastSoundEvent(const QString &soundName, int sourcePlayer)
{
    if (networkGameMode != HostGame || !networkManager.isConnected())
        return;

    QJsonObject message;
    message["type"] = "soundEvent";
    message["sound"] = soundName;
    message["sourcePlayer"] = sourcePlayer;
    networkManager.broadcastMessage(message);
}

QJsonObject GameWindow::buildGameStateMessage() const
{
    QJsonObject message;
    message["type"] = "gameState";
    message["level"] = levelManager.currentLevel();
    message["currentTime"] = levelManager.currentLevelSeconds();
    message["levelOneTime"] = levelManager.levelSeconds(1);
    message["levelTwoTime"] = levelManager.levelSeconds(2);
    message["gameOver"] = state == ResultsState;

    QJsonArray playersArray;
    for (PlayerGuy *player : players)
    {
        QJsonObject playerObject;
        playerObject["number"] = player->getPlayerNumber();
        playerObject["x"] = player->x();
        playerObject["y"] = player->y();
        playerObject["direction"] = player->getFacingDirection();
        playerObject["health"] = player->getHealth();
        playerObject["shield"] = player->isShieldActive();

        // Send teleport-ready data to the joining computer
        playerObject["teleportReady"] = player->canTeleport();
        playerObject["teleportTarget"] = player->getTeleportTargetPlayerIndex();
        playerObject["teleportFrames"] = player->getTeleportFramesRemaining();

        playersArray.append(playerObject);
    }
    message["players"] = playersArray;

    QJsonArray enemiesArray;
    for (BaseEnemy *enemy : enemies)
    {
        QJsonObject enemyObject;
        enemyObject["id"] = enemy->data(9).toInt();
        enemyObject["kind"] = enemy->getKind();
        enemyObject["x"] = enemy->x();
        enemyObject["y"] = enemy->y();
        enemiesArray.append(enemyObject);
    }
    message["enemies"] = enemiesArray;

    QJsonArray coinsArray;
    for (QGraphicsRectItem *coin : coins)
    {
        QJsonObject coinObject;
        coinObject["id"] = coin->data(9).toInt();
        coinObject["x"] = coin->x();
        coinObject["y"] = coin->y();
        coinsArray.append(coinObject);
    }
    message["coins"] = coinsArray;

    QJsonArray projectilesArray;
    for (const Projectile &projectile : projectiles)
    {
        QJsonObject projectileObject;
        projectileObject["id"] = projectile.item->data(9).toInt();
        projectileObject["x"] = projectile.item->x();
        projectileObject["y"] = projectile.item->y();
        projectileObject["owner"] = projectile.ownerIndex;
        projectileObject["enemy"] = projectile.enemyProjectile;
        projectilesArray.append(projectileObject);
    }
    message["projectiles"] = projectilesArray;

    QJsonArray grenadesArray;
    for (const Grenade &grenade : grenades)
    {
        QJsonObject grenadeObject;
        grenadeObject["x"] = grenade.item->x();
        grenadeObject["y"] = grenade.item->y();
        grenadeObject["owner"] = grenade.ownerIndex;
        grenadesArray.append(grenadeObject);
    }
    message["grenades"] = grenadesArray;

    QJsonArray explosionsArray;
    for (QGraphicsEllipseItem *explosion : explosions)
    {
        QJsonObject explosionObject;
        explosionObject["x"] = explosion->x();
        explosionObject["y"] = explosion->y();
        explosionObject["w"] = explosion->rect().width();
        explosionObject["h"] = explosion->rect().height();
        explosionsArray.append(explosionObject);
    }
    message["explosions"] = explosionsArray;

    QJsonArray scoresArray;
    for (int i = 0; i < scoreManager.playerCount(); i++)
    {
        PlayerStats stats = scoreManager.statsForPlayer(i);
        QJsonObject scoreObject;
        scoreObject["name"] = stats.name;
        scoreObject["score"] = stats.score;
        scoreObject["coins"] = stats.coinsCollected;
        scoreObject["kills"] = stats.enemiesKilled;
        scoresArray.append(scoreObject);
    }
    message["scores"] = scoresArray;

    return message;
}

void GameWindow::broadcastGameState()
{
    if (networkGameMode == HostGame && networkManager.isConnected())
    {
        networkManager.broadcastMessage(buildGameStateMessage());
        if (networkFrameCounter % 60 == 0)
        {
            qDebug() << "Game state synchronized";
        }
    }
}

void GameWindow::applyGameState(const QJsonObject &message)
{
    int level = message["level"].toInt(levelManager.currentLevel());

    // Important for clients:
    // If the game state arrives before the local scene/players were created,
    // force the level to load so players, platforms, coins, and enemies exist.
    if (state != PlayingState || players.isEmpty() || levelManager.currentLevel() != level)
    {
        loadLevel(level);
    }

    levelManager.setNetworkTiming(message["currentTime"].toDouble(),
                                  message["levelOneTime"].toDouble(),
                                  message["levelTwoTime"].toDouble());

    applyPlayerState(message["players"].toArray());
    applyEnemyState(message["enemies"].toArray());
    applyCoinState(message["coins"].toArray());
    applyProjectileState(message["projectiles"].toArray());
    applyGrenadeState(message["grenades"].toArray());
    applyExplosionState(message["explosions"].toArray());
    applyScoreState(message["scores"].toArray());

    qDebug() << "Game state synchronized";
}

void GameWindow::applyGrenadeState(const QJsonArray &grenadesArray)
{
    // Client does not simulate grenades.
    // It redraws the grenades that the host sends.
    while (!grenades.isEmpty())
    {
        removeGrenadeAt(grenades.size() - 1);
    }

    for (const QJsonValue &value : grenadesArray)
    {
        QJsonObject grenadeObject = value.toObject();

        double x = grenadeObject["x"].toDouble();
        double y = grenadeObject["y"].toDouble();
        int ownerIndex = grenadeObject["owner"].toInt();

        QGraphicsEllipseItem *grenadeItem = scene->addEllipse(0, 0, 18, 18);
        grenadeItem->setPos(x, y);
        grenadeItem->setBrush(Qt::transparent);
        grenadeItem->setPen(Qt::NoPen);
        grenadeItem->setZValue(85);

        QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, 18, 18, grenadeItem);
        QRadialGradient grad(9, 9, 9);
        grad.setColorAt(0, QColor(150, 150, 150));
        grad.setColorAt(1, QColor(40, 40, 40));
        visual->setBrush(QBrush(grad));
        visual->setPen(QPen(Qt::black, 1));

        QGraphicsRectItem *pin = new QGraphicsRectItem(7, -3, 4, 4, grenadeItem);
        pin->setBrush(QBrush(Qt::gray));
        pin->setPen(QPen(Qt::black, 1));

        Grenade grenade;
        grenade.item = grenadeItem;
        grenade.ownerIndex = ownerIndex;
        grenades.append(grenade);
    }
}

void GameWindow::applyExplosionState(const QJsonArray &explosionsArray)
{
    // Client does not simulate explosions.
    // It redraws the explosions that the host sends.
    while (!explosions.isEmpty())
    {
        QGraphicsEllipseItem *explosion = explosions.takeLast();
        scene->removeItem(explosion);
        delete explosion;
    }

    for (const QJsonValue &value : explosionsArray)
    {
        QJsonObject explosionObject = value.toObject();

        double x = explosionObject["x"].toDouble();
        double y = explosionObject["y"].toDouble();
        double w = explosionObject["w"].toDouble();
        double h = explosionObject["h"].toDouble();

        QGraphicsEllipseItem *explosion = scene->addEllipse(0, 0, w, h);
        explosion->setPos(x, y);
        explosion->setBrush(QBrush(QColor(255, 140, 0, 120)));
        explosion->setPen(QPen(Qt::red, 3));
        explosion->setZValue(90);
        explosion->setData(0, 0);

        explosions.append(explosion);
    }
}

void GameWindow::applyPlayerState(const QJsonArray &playersArray)
{
    for (const QJsonValue &value : playersArray)
    {
        QJsonObject playerObject = value.toObject();
        int index = playerObject["number"].toInt() - 1;

        if (index < 0 || index >= players.size())
            continue;

        players[index]->setNetworkState(QPointF(playerObject["x"].toDouble(),
                                                playerObject["y"].toDouble()),
                                        playerObject["direction"].toInt(1),
                                        playerObject["health"].toInt(3),
                                        playerObject["shield"].toBool());

        // Receive teleport-ready data from the host
        bool teleportReady = playerObject["teleportReady"].toBool(false);
        int teleportTarget = playerObject["teleportTarget"].toInt(-1);
        int teleportFrames = playerObject["teleportFrames"].toInt(0);

        if (teleportReady && teleportTarget >= 0 && teleportFrames > 0)
        {
            players[index]->enableTeleportTo(teleportTarget, teleportFrames);
        }
        else
        {
            players[index]->disableTeleport();
        }
    }
}
BaseEnemy* createEnemyFromKindValue(int kindValue)
{
    BaseEnemy::EnemyKind kind = static_cast<BaseEnemy::EnemyKind>(kindValue);

    switch (kind)
    {
    case BaseEnemy::Fish:
        return new FishEnemy();

    case BaseEnemy::BigFish:
        return new BigEnemy();

    case BaseEnemy::Bird:
        return new BirdEnemy();

    case BaseEnemy::Plant:
        return new PlantEnemy();

    case BaseEnemy::Shark:
        return new SharkEnemy();

    case BaseEnemy::Octopus:
        return new OctopusEnemy();

    case BaseEnemy::FlyingFish:
        return new FlyingFishEnemy();
    }

    return nullptr;
}

void GameWindow::applyEnemyState(const QJsonArray &enemiesArray)
{
    QSet<int> activeIds;

    for (const QJsonValue &value : enemiesArray)
    {
        QJsonObject enemyObject = value.toObject();

        int id = enemyObject["id"].toInt();
        int kindValue = enemyObject["kind"].toInt();
        double enemyX = enemyObject["x"].toDouble();
        double enemyY = enemyObject["y"].toDouble();

        activeIds.insert(id);

        BaseEnemy *matchingEnemy = nullptr;

        for (BaseEnemy *enemy : enemies)
        {
            if (enemy->data(9).toInt() == id)
            {
                matchingEnemy = enemy;
                break;
            }
        }

        // If the client does not have this enemy, create it from the host message.
        if (!matchingEnemy)
        {
            matchingEnemy = createEnemyFromKindValue(kindValue);

            if (matchingEnemy)
            {
                matchingEnemy->setData(9, id);
                scene->addItem(matchingEnemy);
                enemies.append(matchingEnemy);

                qDebug() << "Client created missing enemy id:" << id << "kind:" << kindValue;
            }
        }

        if (matchingEnemy)
        {
            matchingEnemy->setVisible(true);
            matchingEnemy->setPos(enemyX, enemyY);
        }
    }

    for (BaseEnemy *enemy : enemies)
    {
        enemy->setVisible(activeIds.contains(enemy->data(9).toInt()));
    }
}

void GameWindow::applyCoinState(const QJsonArray &coinsArray)
{
    QSet<int> activeIds;

    for (const QJsonValue &value : coinsArray)
    {
        QJsonObject coinObject = value.toObject();
        int id = coinObject["id"].toInt();
        activeIds.insert(id);

        for (QGraphicsRectItem *coin : coins)
        {
            if (coin->data(9).toInt() == id)
            {
                coin->setPos(coinObject["x"].toDouble(), coinObject["y"].toDouble());
                break;
            }
        }
    }

    for (QGraphicsRectItem *coin : coins)
    {
        coin->setVisible(activeIds.contains(coin->data(9).toInt()));
    }
}

void GameWindow::applyProjectileState(const QJsonArray &projectilesArray)
{
    while (!projectiles.isEmpty())
    {
        removeProjectileAt(projectiles.size() - 1);
    }

    for (const QJsonValue &value : projectilesArray)
    {
        QJsonObject projectileObject = value.toObject();
        bool enemyProjectile = projectileObject["enemy"].toBool();
        int owner = projectileObject["owner"].toInt();

        QGraphicsRectItem *bullet = scene->addRect(0, 0, enemyProjectile ? 16 : 20, 6);
        bullet->setPos(projectileObject["x"].toDouble(), projectileObject["y"].toDouble());
        bullet->setBrush(Qt::transparent);
        bullet->setPen(Qt::NoPen);
        bullet->setZValue(80);
        bullet->setData(9, projectileObject["id"].toInt());

        QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, enemyProjectile ? 16 : 20, 6, bullet);
        visual->setBrush(QBrush(enemyProjectile ? Qt::red : (owner == 0 ? Qt::yellow : QColor(255, 180, 40))));
        visual->setPen(QPen(enemyProjectile ? QColor(255, 100, 100) : Qt::white, 1));

        Projectile projectile;
        projectile.item = bullet;
        projectile.ownerIndex = owner;
        projectile.enemyProjectile = enemyProjectile;
        projectiles.append(projectile);
    }
}

void GameWindow::applyScoreState(const QJsonArray &scoresArray)
{
    for (int i = 0; i < scoresArray.size() && i < scoreManager.playerCount(); i++)
    {
        QJsonObject scoreObject = scoresArray[i].toObject();
        PlayerStats stats(scoreObject["name"].toString(QString("Player %1").arg(i + 1)));
        stats.score = scoreObject["score"].toInt();
        stats.coinsCollected = scoreObject["coins"].toInt();
        stats.enemiesKilled = scoreObject["kills"].toInt();
        scoreManager.setStatsForPlayer(i, stats);
    }
}

void GameWindow::updateEnemies()
{
    enemyShootCounter++;

    for (BaseEnemy *enemy : enemies)
    {
        FlyingFishEnemy *flyingFish = dynamic_cast<FlyingFishEnemy*>(enemy);

        if (flyingFish && !players.isEmpty())
        {
            PlayerGuy *target = players[0];
            double bestDistance = qAbs(target->x() - flyingFish->x());

            for (PlayerGuy *player : players)
            {
                double distance = qAbs(player->x() - flyingFish->x());

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    target = player;
                }
            }

            flyingFish->setTargetPosition(target->sceneBoundingRect().center());
        }

        enemy->updateBehavior();
    }

    if (enemyShootCounter >= 75)
    {
        enemyShootCounter = 0;

        for (BaseEnemy *enemy : enemies)
        {
            if (enemy->canShoot())
            {
                shootEnemyBullet(enemy);
            }
        }
    }
}

void GameWindow::shootPlayerBullet(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= players.size())
        return;

    if (playerShootCooldowns[playerIndex] > 0)
        return;

    PlayerGuy *player = players[playerIndex];
    playerShootCooldowns[playerIndex] = 10;

    int direction = player->getFacingDirection();
    QPointF start = player->getGunPoint();

    QGraphicsRectItem *bullet = scene->addRect(0, 0, 20, 6);
    bullet->setPos(start);
    bullet->setBrush(Qt::transparent);
    bullet->setPen(Qt::NoPen);
    bullet->setZValue(80);
    bullet->setData(0, 16.0 * direction);
    bullet->setData(1, 0.0);
    bullet->setData(9, nextProjectileId++);

    QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, 20, 6, bullet);
    visual->setBrush(QBrush(playerIndex == 0 ? Qt::yellow : QColor(255, 180, 40)));
    visual->setPen(QPen(Qt::white, 1));

    Projectile projectile;
    projectile.item = bullet;
    projectile.ownerIndex = playerIndex;
    projectile.enemyProjectile = false;
    projectiles.append(projectile);

    soundManager.playShoot();
    broadcastSoundEvent("shoot", playerIndex + 1);
}

void GameWindow::shootEnemyBullet(BaseEnemy *enemy)
{
    if (!enemy || players.isEmpty())
        return;

    PlayerGuy *target = players[0];
    double bestDistance = qAbs(target->x() - enemy->x());

    for (PlayerGuy *player : players)
    {
        double distance = qAbs(player->x() - enemy->x());
        if (distance < bestDistance)
        {
            bestDistance = distance;
            target = player;
        }
    }

    int direction = target->x() < enemy->x() ? -1 : 1;
    QPointF start = enemy->getGunPoint();

    QGraphicsRectItem *bullet = scene->addRect(0, 0, 16, 6);
    bullet->setPos(start);
    bullet->setBrush(Qt::transparent);
    bullet->setPen(Qt::NoPen);
    bullet->setZValue(80);
    bullet->setData(0, 8.0 * direction);
    bullet->setData(1, 0.0);
    bullet->setData(9, nextProjectileId++);

    QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, 16, 6, bullet);
    visual->setBrush(QBrush(Qt::red));
    visual->setPen(QPen(QColor(255, 100, 100), 1));

    Projectile projectile;
    projectile.item = bullet;
    projectile.ownerIndex = -1;
    projectile.enemyProjectile = true;
    projectiles.append(projectile);
}

bool GameWindow::bulletHitsBlockingItem(QGraphicsRectItem *bullet) const
{
    if (!bullet)
        return false;

    QRectF bulletRect = bullet->sceneBoundingRect();

    for (QGraphicsRectItem *item : blockingItems)
    {
        if (bulletRect.intersects(item->sceneBoundingRect()))
        {
            return true;
        }
    }

    return false;
}

void GameWindow::updateProjectiles()
{
    QRectF sceneBounds = scene->sceneRect();

    for (int i = projectiles.size() - 1; i >= 0; i--)
    {
        QGraphicsRectItem *bullet = projectiles[i].item;
        double vx = bullet->data(0).toDouble();
        double vy = bullet->data(1).toDouble();

        bullet->moveBy(vx, vy);

        if (bulletHitsBlockingItem(bullet) ||
            bullet->x() < sceneBounds.left() ||
            bullet->x() > sceneBounds.right() ||
            bullet->y() < sceneBounds.top() ||
            bullet->y() > sceneBounds.bottom())
        {
            removeProjectileAt(i);
        }
    }
}

void GameWindow::dropGrenade(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= players.size())
        return;

    if (grenadeCooldowns[playerIndex] > 0)
        return;

    PlayerGuy *player = players[playerIndex];
    grenadeCooldowns[playerIndex] = 45;

    int direction = player->getFacingDirection();

    QGraphicsEllipseItem *grenadeItem = scene->addEllipse(0, 0, 18, 18);
    grenadeItem->setPos(player->x() + 20, player->y() + 20);
    grenadeItem->setBrush(Qt::transparent);
    grenadeItem->setPen(Qt::NoPen);
    grenadeItem->setZValue(85);
    grenadeItem->setData(0, direction);
    grenadeItem->setData(1, -8.0);
    grenadeItem->setData(2, 0);
    grenadeItem->setData(3, 3.5);

    QGraphicsEllipseItem *visual = new QGraphicsEllipseItem(0, 0, 18, 18, grenadeItem);
    QRadialGradient grad(9, 9, 9);
    grad.setColorAt(0, QColor(150, 150, 150));
    grad.setColorAt(1, QColor(40, 40, 40));
    visual->setBrush(QBrush(grad));
    visual->setPen(QPen(Qt::black, 1));

    QGraphicsRectItem *pin = new QGraphicsRectItem(7, -3, 4, 4, grenadeItem);
    pin->setBrush(QBrush(Qt::gray));
    pin->setPen(QPen(Qt::black, 1));

    Grenade grenade;
    grenade.item = grenadeItem;
    grenade.ownerIndex = playerIndex;
    grenades.append(grenade);

    // Do NOT play sound here.
    // Explosion sound must only play when the grenade explodes.
}

void GameWindow::updateGrenades()
{
    for (int i = grenades.size() - 1; i >= 0; i--)
    {
        QGraphicsEllipseItem *grenade = grenades[i].item;

        int direction = grenade->data(0).toInt();
        double velocityY = grenade->data(1).toDouble();
        int timerValue = grenade->data(2).toInt();
        double velocityX = grenade->data(3).toDouble();

        velocityY += 0.45;
        timerValue++;

        grenade->moveBy(velocityX * direction, velocityY);

        QRectF grenadeRect = grenade->sceneBoundingRect();

        for (QGraphicsRectItem *platform : platforms)
        {
            if (grenadeRect.intersects(platform->sceneBoundingRect()) && velocityY > 0)
            {
                grenade->setY(platform->sceneBoundingRect().top() - grenade->rect().height());
                velocityY = -5.0;
                velocityX *= 0.65;
                break;
            }
        }

        for (QGraphicsRectItem *item : blockingItems)
        {
            if (grenadeRect.intersects(item->sceneBoundingRect()))
            {
                velocityX *= -0.5;
                break;
            }
        }

        grenade->setData(1, velocityY);
        grenade->setData(2, timerValue);
        grenade->setData(3, velocityX);

        if (timerValue >= 90)
        {
            explodeGrenade(i);
        }
    }
}

bool GameWindow::pointInsideExplosion(QPointF point, QPointF explosionCenter, double radius) const
{
    double dx = point.x() - explosionCenter.x();
    double dy = point.y() - explosionCenter.y();
    double distance = qSqrt(dx * dx + dy * dy);

    return distance <= radius;
}

bool GameWindow::rectIntersectsExplosion(const QRectF &rect, QPointF center, double radius) const
{
    double closestX = qMax(rect.left(), qMin(center.x(), rect.right()));
    double closestY = qMax(rect.top(), qMin(center.y(), rect.bottom()));

    double dx = center.x() - closestX;
    double dy = center.y() - closestY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

void GameWindow::explodeGrenade(int grenadeIndex)
{
    if (grenadeIndex < 0 || grenadeIndex >= grenades.size())
        return;

    QGraphicsEllipseItem *grenade = grenades[grenadeIndex].item;
    int ownerIndex = grenades[grenadeIndex].ownerIndex;
    QPointF center = grenade->sceneBoundingRect().center();
    double radius = 120.0;

    QGraphicsEllipseItem *explosion = scene->addEllipse(0, 0, radius * 2, radius * 2);
    explosion->setPos(center.x() - radius, center.y() - radius);
    explosion->setBrush(QBrush(QColor(255, 140, 0, 120)));
    explosion->setPen(QPen(Qt::red, 3));
    explosion->setZValue(90);
    explosion->setData(0, 0);
    explosions.append(explosion);

    removeGrenadeAt(grenadeIndex);

    soundManager.playExplosion();
    broadcastSoundEvent("explosion", ownerIndex + 1);

    for (int i = enemies.size() - 1; i >= 0; i--)
    {
        BaseEnemy *enemy = enemies[i];

        if (rectIntersectsExplosion(enemy->sceneBoundingRect(), center, radius))
        {
            awardEnemyKill(ownerIndex, enemy);
            scene->removeItem(enemy);
            delete enemy;
            enemies.removeAt(i);
        }
    }

    for (int i = projectiles.size() - 1; i >= 0; i--)
    {
        if (projectiles[i].enemyProjectile &&
            rectIntersectsExplosion(projectiles[i].item->sceneBoundingRect(), center, radius))
        {
            removeProjectileAt(i);
        }
    }
}

void GameWindow::updateExplosions()
{
    for (int i = explosions.size() - 1; i >= 0; i--)
    {
        QGraphicsEllipseItem *explosion = explosions[i];
        int timerValue = explosion->data(0).toInt();
        timerValue++;
        explosion->setData(0, timerValue);

        if (timerValue > 12)
        {
            scene->removeItem(explosion);
            delete explosion;
            explosions.removeAt(i);
        }
    }
}

void GameWindow::checkCollisions()
{
    for (int playerIndex = 0; playerIndex < players.size(); playerIndex++)
    {
        PlayerGuy *player = players[playerIndex];
        QRectF playerRect = player->sceneBoundingRect();

        for (int i = coins.size() - 1; i >= 0; i--)
        {
            if (playerRect.intersects(coins[i]->sceneBoundingRect()))
            {
                int coinId = coins[i]->data(9).toInt();
                scene->removeItem(coins[i]);
                delete coins[i];
                coins.removeAt(i);
                scoreManager.addCoin(playerIndex, 10);
                soundManager.playCoin();

                if (networkGameMode == HostGame)
                {
                    QJsonObject message;
                    message["type"] = "coinCollected";
                    message["coinId"] = coinId;
                    message["player"] = playerIndex + 1;
                    networkManager.broadcastMessage(message);
                    broadcastSoundEvent("coin", playerIndex + 1);
                }
            }
        }

        for (Obstacle *obstacle : obstacles)
        {
            if (playerRect.intersects(obstacle->sceneBoundingRect()))
            {
                obstacle->triggerEffect(player);
            }
        }

        // Spikes kill/reset the player immediately
        for (QGraphicsPolygonItem *spike : spikes)
        {
            if (playerRect.intersects(spike->sceneBoundingRect()))
            {
                resetPlayerToSpawn(playerIndex);
                return;
            }
        }

        for (BaseEnemy *enemy : enemies)
        {
            if (playerRect.intersects(enemy->sceneBoundingRect()))
            {
                damagePlayer(playerIndex);
                break;
            }
        }

        // SINGLE PLAYER LEVEL 1: touching friendly completes level 1
        if (!coopMode && levelManager.currentLevel() == 1 && friendlyTrigger &&
            playerRect.intersects(friendlyTrigger->sceneBoundingRect()))
        {
            shieldUnlockedForLevel2 = true;

            QMessageBox::information(this,
                                     "Friendly",
                                     "Well done you have completed level 1.\nYou received a shield for level 2.");

            completeLevel();
            return;
        }

        // Normal exit for non-level-1 single player
        if (!coopMode && finishFlag && levelManager.currentLevel() != 1 &&
            playerRect.intersects(finishFlag->sceneBoundingRect()))
        {
            completeLevel();
            return;
        }
    }

    // MULTIPLAYER LEVEL 1: both players must reach the friendly
    if (coopMode && players.size() == 2 && levelManager.currentLevel() == 1 && friendlyTrigger)
    {
        bool p1Reached = players[0]->sceneBoundingRect().intersects(friendlyTrigger->sceneBoundingRect());
        bool p2Reached = players[1]->sceneBoundingRect().intersects(friendlyTrigger->sceneBoundingRect());

        if (p1Reached && p2Reached)
        {
            shieldUnlockedForLevel2 = true;

            QMessageBox::information(this,
                                     "Friendly",
                                     "Well done you have completed level 1.\nYou received a shield for level 2.");

            completeLevel();
            return;
        }
    }

    if (coopMode && players.size() == 2 && finishFlag && levelManager.currentLevel() != 1)
    {
        bool p1Finished = players[0]->sceneBoundingRect().intersects(finishFlag->sceneBoundingRect());
        bool p2Finished = players[1]->sceneBoundingRect().intersects(finishFlag->sceneBoundingRect());

        if (finishZoneP1 && finishZoneP2)
        {
            p1Finished = players[0]->sceneBoundingRect().intersects(finishZoneP1->sceneBoundingRect()) ||
                         players[0]->sceneBoundingRect().intersects(finishZoneP2->sceneBoundingRect());
            p2Finished = players[1]->sceneBoundingRect().intersects(finishZoneP1->sceneBoundingRect()) ||
                         players[1]->sceneBoundingRect().intersects(finishZoneP2->sceneBoundingRect());
        }

        if (p1Finished && p2Finished)
        {
            completeLevel();
            return;
        }
    }

    for (int i = projectiles.size() - 1; i >= 0; i--)
    {
        Projectile projectile = projectiles[i];
        QRectF bulletRect = projectile.item->sceneBoundingRect();

        if (projectile.enemyProjectile)
        {
            for (int playerIndex = 0; playerIndex < players.size(); playerIndex++)
            {
                PlayerGuy *player = players[playerIndex];

                if (player->isShieldActive() && bulletRect.intersects(player->getShieldSceneRect()))
                {
                    removeProjectileAt(i);
                    break;
                }

                if (bulletRect.intersects(player->sceneBoundingRect()))
                {
                    removeProjectileAt(i);
                    damagePlayer(playerIndex);
                    break;
                }
            }
        }
        else
        {
            bool bulletUsed = false;

            for (int enemyIndex = enemies.size() - 1; enemyIndex >= 0; enemyIndex--)
            {
                BaseEnemy *enemy = enemies[enemyIndex];

                if (bulletRect.intersects(enemy->sceneBoundingRect()))
                {
                    removeProjectileAt(i);
                    awardEnemyKill(projectile.ownerIndex, enemy);

                    scene->removeItem(enemy);
                    delete enemy;
                    enemies.removeAt(enemyIndex);

                    bulletUsed = true;
                    break;
                }
            }

            if (bulletUsed)
            {
                continue;
            }
        }
    }
}

void GameWindow::damagePlayer(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= players.size())
        return;

    if (damageCooldowns[playerIndex] > 0)
        return;

    damageCooldowns[playerIndex] = 45;
    players[playerIndex]->takeDamage(1);

    if (players[playerIndex]->isDead())
    {
        resetPlayerToSpawn(playerIndex);
    }
}

bool GameWindow::tryTeleportPlayer(int playerIndex)
{
    if (!coopMode || playerIndex < 0 || playerIndex >= players.size())
        return false;

    PlayerGuy *teleportingPlayer = players[playerIndex];

    if (!teleportingPlayer->canTeleport())
        return false;

    int targetIndex = teleportingPlayer->getTeleportTargetPlayerIndex();

    if (targetIndex < 0 || targetIndex >= players.size() || targetIndex == playerIndex)
    {
        teleportingPlayer->disableTeleport();
        return false;
    }

    PlayerGuy *targetPlayer = players[targetIndex];
    QPointF safePosition = findSafeTeleportPosition(teleportingPlayer, targetPlayer);
    teleportingPlayer->setPos(safePosition);
    teleportingPlayer->disableTeleport();

    return true;
}

QPointF GameWindow::findSafeTeleportPosition(PlayerGuy *teleportingPlayer, PlayerGuy *targetPlayer) const
{
    QList<QPointF> candidates;
    candidates << QPointF(targetPlayer->x() + 64, targetPlayer->y())
               << QPointF(targetPlayer->x() - 64, targetPlayer->y())
               << QPointF(targetPlayer->x() + 50, targetPlayer->y() - 70)
               << QPointF(targetPlayer->x() - 50, targetPlayer->y() - 70)
               << QPointF(targetPlayer->x(), targetPlayer->y() - 82);

    for (const QPointF &candidate : candidates)
    {
        if (candidate.x() < 0 ||
            candidate.x() + teleportingPlayer->rect().width() > scene->sceneRect().width() ||
            candidate.y() < 0 ||
            candidate.y() + teleportingPlayer->rect().height() > scene->sceneRect().height())
        {
            continue;
        }

        if (!positionIntersectsPlatform(teleportingPlayer, candidate))
        {
            return candidate;
        }
    }

    return QPointF(targetPlayer->x(), targetPlayer->y() - 82);
}

bool GameWindow::positionIntersectsPlatform(PlayerGuy *player, const QPointF &position) const
{
    QRectF candidateRect(position, player->rect().size());

    for (QGraphicsRectItem *platform : platforms)
    {
        if (candidateRect.intersects(platform->sceneBoundingRect()))
        {
            return true;
        }
    }

    return false;
}

void GameWindow::resetPlayerToSpawn(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= players.size())
        return;

    players[playerIndex]->resetPosition();
    damageCooldowns[playerIndex] = 45;
    respawnEnemies();
}

void GameWindow::resetCurrentLevel()
{
    loadLevel(levelManager.currentLevel());
}

void GameWindow::removeProjectileAt(int index)
{
    if (index < 0 || index >= projectiles.size())
        return;

    QGraphicsRectItem *bullet = projectiles[index].item;
    scene->removeItem(bullet);
    delete bullet;
    projectiles.removeAt(index);
}

void GameWindow::removeGrenadeAt(int index)
{
    if (index < 0 || index >= grenades.size())
        return;

    QGraphicsEllipseItem *grenade = grenades[index].item;
    scene->removeItem(grenade);
    delete grenade;
    grenades.removeAt(index);
}

void GameWindow::awardEnemyKill(int playerIndex, BaseEnemy *enemy)
{
    if (!enemy)
        return;

    scoreManager.addEnemyKill(playerIndex, enemy->getScoreValue());

    if (networkGameMode == HostGame)
    {
        QJsonObject message;
        message["type"] = "enemyKilled";
        message["enemyId"] = enemy->data(9).toInt();
        message["player"] = playerIndex + 1;
        networkManager.broadcastMessage(message);
    }
}

void GameWindow::completeLevel()
{
    levelManager.finishCurrentLevel();
    soundManager.playLevelComplete();
    broadcastSoundEvent("levelComplete", 0);

    if (coopMode)
    {
        if (levelManager.hasNextLevel())
        {
            levelManager.advanceLevel();

            if (networkGameMode == HostGame)
            {
                QJsonObject levelMessage;
                levelMessage["type"] = "levelChanged";
                levelMessage["level"] = levelManager.currentLevel();
                networkManager.broadcastMessage(levelMessage);
            }

            loadLevel(levelManager.currentLevel());
            return;
        }

        for (int i = 0; i < scoreManager.playerCount(); i++)
        {
            scoreManager.addBonus(i, MultiplayerManager::CompletionBonusPoints);
        }

        if (networkGameMode == HostGame)
        {
            QJsonObject gameOverMessage = buildGameStateMessage();
            gameOverMessage["type"] = "gameOver";
            networkManager.broadcastMessage(gameOverMessage);
        }

        logResultsToFile();
        showResultsScreen();
        return;
    }

    if (levelManager.hasNextLevel())
    {
        levelManager.advanceLevel();
        loadLevel(levelManager.currentLevel());
        return;
    }

    logResultsToFile();
    showResultsScreen();
}

void GameWindow::logResultsToFile()
{
    QFile file("multiplayer_results.log");

    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream << "Match finished: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    stream << "Mode: " << (coopMode ? "Multiplayer" : "Single Player") << "\n";

    for (int i = 0; i < scoreManager.playerCount(); i++)
    {
        PlayerStats stats = scoreManager.statsForPlayer(i);
        stream << stats.name
               << " | time=" << (coopMode ? levelManager.levelSeconds(1) : levelManager.totalSeconds())
               << " | points=" << stats.score
               << " | enemiesKilled=" << stats.enemiesKilled
               << " | coinsCollected=" << stats.coinsCollected
               << "\n";
    }

    stream << "Team score=" << scoreManager.teamScore() << "\n\n";
}

void GameWindow::updateHud()
{
    if (!timerText || !scoreText || !levelText)
        return;

    timerText->setPlainText(QString("Level time: %1  Total: %2")
                            .arg(levelManager.currentLevelSeconds(), 0, 'f', 2)
                            .arg(levelManager.totalSeconds() + levelManager.currentLevelSeconds(), 0, 'f', 2));

    QString scoreLine;
    for (int i = 0; i < scoreManager.playerCount(); i++)
    {
        PlayerStats stats = scoreManager.statsForPlayer(i);
        if (!scoreLine.isEmpty())
        {
            scoreLine += " | ";
        }
        scoreLine += QString("P%1 Score: %2 Coins: %3 Kills: %4")
                     .arg(i + 1)
                     .arg(stats.score)
                     .arg(stats.coinsCollected)
                     .arg(stats.enemiesKilled);
    }

    if (scoreManager.playerCount() == 2)
    {
        scoreLine += QString(" | Team: %1").arg(scoreManager.teamScore());
    }

    scoreText->setPlainText(scoreLine);
    if (coopMode)
    {
        QString role = "Local";
        if (networkGameMode == HostGame)
        {
            role = "Host";
        }
        else if (networkGameMode == ClientGame)
        {
            role = "Client";
        }

        levelText->setPlainText(QString("%1 Multiplayer Co-op Obstacle Course").arg(role));
    }
    else
    {
        levelText->setPlainText(QString("Level %1: %2")
                                .arg(levelManager.currentLevel())
                                .arg(levelManager.currentLevel() == 1 ? "Jungle" : "Ocean"));
    }

    if (teleportText)
    {
        QString teleportLine;

        for (int i = 0; i < players.size(); i++)
        {
            if (players[i]->canTeleport())
            {
                if (!teleportLine.isEmpty())
                {
                    teleportLine += " | ";
                }

                teleportLine += QString("P%1 TELEPORT READY! Press T  (%2s)")
                                .arg(i + 1)
                                .arg(qCeil(players[i]->getTeleportFramesRemaining() / 60.0));
            }
        }

        teleportText->setPlainText(teleportLine);
    }
}

void GameWindow::updateCamera()
{
    if (state != PlayingState || players.isEmpty())
        return;

    PlayerGuy *focusPlayer = players[0];

    for (PlayerGuy *player : players)
    {
        if (player->x() > focusPlayer->x())
        {
            focusPlayer = player;
        }
    }

    centerOn(focusPlayer);

    QPointF topLeft = mapToScene(viewport()->rect().topLeft());

    levelText->setPos(topLeft.x() + 20, topLeft.y() + 15);
    timerText->setPos(topLeft.x() + 20, topLeft.y() + 45);
    scoreText->setPos(topLeft.x() + 20, topLeft.y() + 75);
    controlsText->setPos(topLeft.x() + 20, topLeft.y() + 105);

    if (teleportText)
    {
        teleportText->setPos(topLeft.x() + 20, topLeft.y() + 150);
    }

    if (networkStatusText)
    {
        networkStatusText->setPos(topLeft.x() + 20, topLeft.y() + 180);
    }
}

void GameWindow::showResultsScreen()
{
    state = ResultsState;
    clearSceneData();
    keysPressed.clear();

    scene->setSceneRect(0, 0, 1000, 650);
    scene->setBackgroundBrush(QBrush(QColor(35, 110, 130)));

    QGraphicsTextItem *title = scene->addText("End Game Results");
    title->setDefaultTextColor(Qt::white);
    title->setFont(QFont("Arial", 32, QFont::Bold));
    title->setPos(310, 65);
    title->setZValue(10);

    QString results;
    for (int i = 0; i < scoreManager.playerCount(); i++)
    {
        PlayerStats stats = scoreManager.statsForPlayer(i);
        results += QString("%1\nFinal score: %2\nCoins collected: %3\nEnemies killed: %4\n\n")
                   .arg(stats.name)
                   .arg(stats.score)
                   .arg(stats.coinsCollected)
                   .arg(stats.enemiesKilled);
    }

    if (coopMode)
    {
        results += QString("Co-op completion time: %1 seconds\n")
                   .arg(levelManager.levelSeconds(1), 0, 'f', 2);
    }
    else
    {
        results += QString("Level 1 completion time: %1 seconds\n")
                   .arg(levelManager.levelSeconds(1), 0, 'f', 2);
        results += QString("Level 2 completion time: %1 seconds\n")
                   .arg(levelManager.levelSeconds(2), 0, 'f', 2);
        results += QString("Total game time: %1 seconds\n")
                   .arg(levelManager.totalSeconds(), 0, 'f', 2);
    }

    if (scoreManager.playerCount() == 2)
    {
        results += QString("Combined team score: %1\n").arg(scoreManager.teamScore());
    }

    results += "\nPress R or click Restart to return to the menu.";

    QGraphicsTextItem *resultsText = scene->addText(results);
    resultsText->setDefaultTextColor(Qt::white);
    resultsText->setFont(QFont("Consolas", 15, QFont::Bold));
    resultsText->setPos(250, 145);
    resultsText->setZValue(10);

    addMenuButton("Restart", "restart", 105, 545);
    addMenuButton("Main Menu", "mainmenu", 365, 545);
    addMenuButton("Quit", "quit", 625, 545);

    centerOn(500, 325);
}

void GameWindow::updateOceanBackground()
{
    if (levelManager.currentLevel() != 2 || state != PlayingState)
    {
        return;
    }

    double time = levelManager.currentLevelSeconds();

    // 1. Move background whale
    if (backgroundWhale)
    {
        double newX = backgroundWhale->x() + 0.6; // slow horizontal swimming
        if (newX > 3200)
        {
            newX = -400; // wrap around to left
        }
        // Gentle vertical sine wave motion
        double newY = 150 + 25 * qSin(time * 0.4);
        backgroundWhale->setPos(newX, newY);
    }

    // 2. Spawn new bubbles randomly (approx once every 0.6 seconds / 36 frames)
    if (QRandomGenerator::global()->bounded(36) == 0)
    {
        QImage img(":/GUI/Bubble.png");
        if (!img.isNull())
        {
            QPixmap bubblePix = QPixmap::fromImage(img);
            
            // Random bubble size between 12 and 24 pixels
            int size = QRandomGenerator::global()->bounded(12, 25);
            bubblePix = bubblePix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            QGraphicsPixmapItem *bubbleVisual = scene->addPixmap(bubblePix);
            
            // Spawn at random X along the ground (0 to 3000)
            double spawnX = QRandomGenerator::global()->bounded(3000);
            double spawnY = 575; // just above ground level (y=580)
            
            bubbleVisual->setPos(spawnX, spawnY);
            bubbleVisual->setZValue(4); // Float in front of seaweed (2) and platforms (1) but behind players (50)

            BubbleData bubble;
            bubble.item = bubbleVisual;
            bubble.speedY = 1.0 + QRandomGenerator::global()->bounded(15) / 10.0; // speed between 1.0 and 2.5 pixels/frame
            bubble.initialX = spawnX;
            bubble.waveFrequency = 0.05 + QRandomGenerator::global()->bounded(10) / 100.0; // frequency of wobble
            bubble.waveAmplitude = 5.0 + QRandomGenerator::global()->bounded(15); // amplitude of wobble (5 to 20 pixels)
            bubble.phase = QRandomGenerator::global()->bounded(314) / 100.0; // random starting phase

            oceanBubbles.append(bubble);
        }
    }

    // 3. Update existing bubbles
    for (int i = oceanBubbles.size() - 1; i >= 0; --i)
    {
        BubbleData &bubble = oceanBubbles[i];
        
        // Move bubble up
        double newY = bubble.item->y() - bubble.speedY;
        
        // Horizontal oscillation (drift)
        double newX = bubble.initialX + bubble.waveAmplitude * qSin(time * bubble.waveFrequency * 6.0 + bubble.phase);
        
        bubble.item->setPos(newX, newY);

        // Delete if it goes off screen (e.g. above y = -50)
        if (newY < -50)
        {
            scene->removeItem(bubble.item);
            delete bubble.item;
            oceanBubbles.removeAt(i);
        }
    }
}

void GameWindow::respawnEnemies()
{
    // 1. Remove and delete all active enemies
    for (BaseEnemy *enemy : enemies)
    {
        scene->removeItem(enemy);
        delete enemy;
    }
    enemies.clear();

    // 2. Re-create enemies from levelEnemySpawns cache
    for (const EnemySpawnInfo &spawn : levelEnemySpawns)
    {
        BaseEnemy *enemy = nullptr;
        switch (spawn.type)
        {
        case BaseEnemy::Fish:
            enemy = new FishEnemy();
            break;
        case BaseEnemy::BigFish:
            enemy = new BigEnemy();
            break;
        case BaseEnemy::Bird:
            enemy = new BirdEnemy();
            break;
        case BaseEnemy::FlyingFish:
            enemy = new FlyingFishEnemy();
            break;
        case BaseEnemy::Plant:
            enemy = new PlantEnemy();
            break;
        case BaseEnemy::Shark:
            enemy = new SharkEnemy();
            break;
        case BaseEnemy::Octopus:
            enemy = new OctopusEnemy();
            break;
        }

        if (enemy)
        {
            enemy->setPos(spawn.x, spawn.y);
            enemy->setData(9, nextEnemyId++);
            scene->addItem(enemy);
            enemies.append(enemy);
        }
    }
}
