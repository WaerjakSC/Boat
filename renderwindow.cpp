#include "renderwindow.h"
#include "innpch.h"
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <QOpenGLFunctions>
#include <QStatusBar>
#include <QTimer>
#include <chrono>

#include "boat.h"
#include "colorshader.h"
#include "mainwindow.h"
#include "objmesh.h"
#include "textureshader.h"

RenderWindow::RenderWindow(const QSurfaceFormat &format, MainWindow *mainWindow)
    : mContext(nullptr), mMainWindow(mainWindow)
{
    //This is sent to QWindow:
    setSurfaceType(QWindow::OpenGLSurface);
    setFormat(format);
    //Make the OpenGL context
    mContext = new QOpenGLContext(this);
    //Give the context the wanted OpenGL format (v4.1 Core)
    mContext->setFormat(requestedFormat());
    if (!mContext->create()) {
        delete mContext;
        mContext = nullptr;
        qDebug() << "Context could not be made - quitting this application";
    }

    //Make the gameloop timer:
    mRenderTimer = new QTimer(this);
}

RenderWindow::~RenderWindow()
{
    for (auto &i : mShaderProgram) {
        delete i;
    }
}

/// Sets up the general OpenGL stuff and the buffers needed to render a triangle
void RenderWindow::init()
{
    //Connect the gameloop timer to the render function:
    connect(mRenderTimer, SIGNAL(timeout()), this, SLOT(render()));

    //********************** General OpenGL stuff **********************

    //The OpenGL context has to be set.
    //The context belongs to the instance of this class!
    if (!mContext->makeCurrent(this)) {
        qDebug() << "makeCurrent() failed";
        return;
    }

    //just to make sure we don't init several times
    //used in exposeEvent()
    if (!mInitialized)
        mInitialized = true;

    //must call this to use OpenGL functions
    initializeOpenGLFunctions();

    //Print render version info:
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;

    //Start the Qt OpenGL debugger
    //Really helpfull when doing OpenGL
    //Supported on most Windows machines
    //reverts to plain glGetError() on Mac and other unsupported PCs
    // - can be deleted
    startOpenGLDebugger();

    //general OpenGL stuff:
    glEnable(GL_DEPTH_TEST);              //enables depth sorting - must use GL_DEPTH_BUFFER_BIT in glClear
    glEnable(GL_CULL_FACE);               //draws only front side of models - usually what you want -
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f); //color used in glClear GL_COLOR_BUFFER_BIT

    //Compile shaders:
    mShaderProgram[0] = new ColorShader("plainshader");
    qDebug() << "Plain shader program id: " << mShaderProgram[0]->getProgram();

    mShaderProgram[1] = new TextureShader("textureshader");
    qDebug() << "Texture shader program id: " << mShaderProgram[1]->getProgram();

    //**********************  Texture stuff: **********************

    mTexture[0] = new Texture("white.bmp");
    mTexture[1] = new Texture("hund.bmp", 1);
    //Set the textures loaded to a texture unit
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture[0]->id());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTexture[1]->id());

    //********************** Making the objects to be drawn **********************

    MakePlane();

    mBoat = new Boat(gsl::Vector3D(0.f, 10.f, 0.f));
    mBoat->init();
    mBoat->setShader(mShaderProgram[0]);
    mBoat->mName = "boat";
    mBoat->mRenderWindow = this;
    mBoat->mMaterial.setTextureUnit(0);
    mBoat->mMaterial.mObjectColor = gsl::Vector3D(0.1f, 0.1f, 0.8f);
    mVisualObjects.push_back(mBoat);

    //********************** Set up camera **********************
    mCurrentCamera = new Camera();
    mCurrentCamera->setPosition(gsl::Vector3D(0.f, 30.f, 0.f));
    mCurrentCamera->pitch(90.f);

    //new system - shader sends uniforms so needs to get the view and projection matrixes from camera
    mShaderProgram[0]->setCurrentCamera(mCurrentCamera);
    mShaderProgram[1]->setCurrentCamera(mCurrentCamera);
}

///Called each frame - doing the rendering
void RenderWindow::render()
{
    float time{static_cast<float>(mTime.elapsed()) / 1000.f};
    float dt{time - mLastFrameTime};
    mLastFrameTime = time;
    //input
    handleInput(dt);

    mCurrentCamera->update();

    mTimeStart.restart();        //restart FPS clock
    mContext->makeCurrent(this); //must be called every frame (every time mContext->swapBuffers is called)

    //to clear the screen for each redraw
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto visObject : mVisualObjects) {
        visObject->draw();
        //        checkForGLerrors();
    }
    mBoat->Tick(dt);
    gsl::Vector3D NewCamPos{mBoat->position()};
    NewCamPos.setY(30.f);
    mCurrentCamera->setPosition(NewCamPos);

    //Calculate framerate before
    // checkForGLerrors() because that takes a long time
    // and before swapBuffers(), else it will show the vsync time
    calculateFramerate();

    //Qt require us to call this swapBuffers() -function.
    // swapInterval is 1 by default which means that swapBuffers() will (hopefully) block
    // and wait for vsync.
    mContext->swapBuffers(this);
}

void RenderWindow::setupPlainShader(int shaderIndex)
{
    mMatrixUniform0 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "mMatrix");
    vMatrixUniform0 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "vMatrix");
    pMatrixUniform0 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "pMatrix");
}

void RenderWindow::setupTextureShader(int shaderIndex)
{
    mMatrixUniform1 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "mMatrix");
    vMatrixUniform1 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "vMatrix");
    pMatrixUniform1 = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "pMatrix");
    mTextureUniform = glGetUniformLocation(mShaderProgram[shaderIndex]->getProgram(), "textureSampler");
}

void RenderWindow::MakePlane()
{
    VisualObject *temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(0, 0, 0);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    // Adding some more 2D squares -- would benefit from a resource manager here
    // if plane.obj was a larger file, since we're essentially
    // re-reading the plane.obj file each time we create a new object.
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(300, 0, 300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(00, 0, 300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(300, 0, 0);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(-300, 0, 0);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(0, 0, -300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(-300, 0, 300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(300, 0, -300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
    temp = new ObjMesh("plane.obj");
    temp->init();
    temp->setShader(mShaderProgram[1]);
    temp->mMaterial.setTextureUnit(1);
    //temp->mMaterial.mObjectColor = gsl::Vector3D(0.0f, 0.0f, 0.f);
    temp->mMatrix.setPosition(-300, 0, -300);
    temp->mMatrix.scale(gsl::Vector3D(150.f, 1.f, 150.f));
    mVisualObjects.push_back(temp);
}

//This function is called from Qt when window is exposed (shown)
//and when it is resized
//exposeEvent is a overridden function from QWindow that we inherit from
void RenderWindow::exposeEvent(QExposeEvent *)
{
    if (!mInitialized) {
        init();
        mTime.start();
    }

    //This is just to support modern screens with "double" pixels
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, static_cast<GLint>(width() * retinaScale), static_cast<GLint>(height() * retinaScale));

    //If the window actually is exposed to the screen we start the main loop
    //isExposed() is a function in QWindow
    if (isExposed()) {
        //This timer runs the actual MainLoop
        //16 means 16ms = 60 Frames pr second (should be 16.6666666 to be exact..)
        mRenderTimer->start(1);
        mTimeStart.start();
    }
    mAspectratio = static_cast<float>(width()) / height();
    //    qDebug() << mAspectratio;
    mCurrentCamera->mProjectionMatrix.perspective(45.f, mAspectratio, 1.f, 100.f);
    //    qDebug() << mCamera.mProjectionMatrix;
}

//Simple way to turn on/off wireframe mode
//Not totally accurate, but draws the objects with
//lines instead of filled polygons
void RenderWindow::toggleWireframe()
{
    mWireframe = !mWireframe;
    if (mWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //turn on wireframe mode
        glDisable(GL_CULL_FACE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //turn off wireframe mode
        glEnable(GL_CULL_FACE);
    }
}

//The way this is set up is that we start the clock before doing the draw call,
//and check the time right after it is finished (done in the render function)
//This will approximate what framerate we COULD have.
//The actual frame rate on your monitor is limited by the vsync and is probably 60Hz
void RenderWindow::calculateFramerate()
{
    long long nsecElapsed = mTimeStart.nsecsElapsed();
    static int frameCount{0}; //counting actual frames for a quick "timer" for the statusbar

    if (mMainWindow) //if no mainWindow, something is really wrong...
    {
        ++frameCount;
        if (frameCount > 30) //once pr 30 frames = update the message twice pr second (on a 60Hz monitor)
        {
            //showing some statistics in status bar
            mMainWindow->statusBar()->showMessage(" Boat Position: " +
                                                  QString::number(nsecElapsed / 1000000., 'g', 4) + " ms  |  " +
                                                  "FPS (approximated): " + QString::number(1E9 / nsecElapsed, 'g', 7));
            frameCount = 0; //reset to show a new message in 60 frames
        }
    }
}

/// Uses QOpenGLDebugLogger if this is present
/// Reverts to glGetError() if not
void RenderWindow::checkForGLerrors()
{
    if (mOpenGLDebugLogger) {
        const QList<QOpenGLDebugMessage> messages = mOpenGLDebugLogger->loggedMessages();
        for (const QOpenGLDebugMessage &message : messages)
            qDebug() << message;
    }
    else {
        GLenum err = GL_NO_ERROR;
        while ((err = glGetError()) != GL_NO_ERROR) {
            qDebug() << "glGetError returns " << err;
        }
    }
}

/// Tries to start the extended OpenGL debugger that comes with Qt
void RenderWindow::startOpenGLDebugger()
{
    QOpenGLContext *temp = this->context();
    if (temp) {
        QSurfaceFormat format = temp->format();
        if (!format.testOption(QSurfaceFormat::DebugContext))
            qDebug() << "This system can not use QOpenGLDebugLogger, so we revert to glGetError()";

        if (temp->hasExtension(QByteArrayLiteral("GL_KHR_debug"))) {
            qDebug() << "System can log OpenGL errors!";
            mOpenGLDebugLogger = new QOpenGLDebugLogger(this);
            if (mOpenGLDebugLogger->initialize()) // initializes in the current context
                qDebug() << "Started OpenGL debug logger!";
        }

        if (mOpenGLDebugLogger)
            mOpenGLDebugLogger->disableMessages(QOpenGLDebugMessage::APISource, QOpenGLDebugMessage::OtherType, QOpenGLDebugMessage::NotificationSeverity);
    }
}

void RenderWindow::setCameraSpeed(float value)
{
    mCameraSpeed += value;

    //Keep within min and max values
    if (mCameraSpeed < 0.01f)
        mCameraSpeed = 0.01f;
    if (mCameraSpeed > 0.3f)
        mCameraSpeed = 0.3f;
}

void RenderWindow::handleInput(float deltaTime)
{
    //Camera
    mCurrentCamera->setSpeed(0.f); //cancel last frame movement
    if (mInput.RMB) {
        if (mInput.W)
            mCurrentCamera->setSpeed(-mCameraSpeed);
        if (mInput.S)
            mCurrentCamera->setSpeed(mCameraSpeed);
        if (mInput.D)
            mCurrentCamera->moveRight(mCameraSpeed);
        if (mInput.A)
            mCurrentCamera->moveRight(-mCameraSpeed);
        if (mInput.Q)
            mCurrentCamera->updateHeight(-mCameraSpeed);
        if (mInput.E)
            mCurrentCamera->updateHeight(mCameraSpeed);
    }
    else {
        if (mInput.W)
            mBoat->MoveInput(Qt::Key_W, deltaTime);
        if (mInput.S)
            mBoat->MoveInput(Qt::Key_S, deltaTime);
        if (mInput.D)
            mBoat->MoveInput(Qt::Key_D, deltaTime);
        if (mInput.A)
            mBoat->MoveInput(Qt::Key_A, deltaTime);
        //        if (mInput.Q)
        //            mBoat->mMatrix.translateY(mCameraSpeed);
        //        if (mInput.E)
        //            mBoat->mMatrix.translateY(-mCameraSpeed);
    }
}

void RenderWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) //Shuts down whole program
    {
        mMainWindow->close();
    }

    //    You get the keyboard input like this
    if (event->key() == Qt::Key_W) {
        mInput.W = true;
    }
    if (event->key() == Qt::Key_S) {
        mInput.S = true;
    }
    if (event->key() == Qt::Key_D) {
        mInput.D = true;
    }
    if (event->key() == Qt::Key_A) {
        mInput.A = true;
    }
    if (event->key() == Qt::Key_Q) {
        mInput.Q = true;
    }
    if (event->key() == Qt::Key_E) {
        mInput.E = true;
    }
    if (event->key() == Qt::Key_Z) {
    }
    if (event->key() == Qt::Key_X) {
    }
    if (event->key() == Qt::Key_Up) {
        mInput.UP = true;
    }
    if (event->key() == Qt::Key_Down) {
        mInput.DOWN = true;
    }
    if (event->key() == Qt::Key_Left) {
        mInput.LEFT = true;
    }
    if (event->key() == Qt::Key_Right) {
        mInput.RIGHT = true;
    }
    if (event->key() == Qt::Key_U) {
    }
    if (event->key() == Qt::Key_O) {
    }
}

void RenderWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_W) {
        mInput.W = false;
    }
    if (event->key() == Qt::Key_S) {
        mInput.S = false;
    }
    if (event->key() == Qt::Key_D) {
        mInput.D = false;
    }
    if (event->key() == Qt::Key_A) {
        mInput.A = false;
    }
    if (event->key() == Qt::Key_Q) {
        mInput.Q = false;
    }
    if (event->key() == Qt::Key_E) {
        mInput.E = false;
    }
    if (event->key() == Qt::Key_Z) {
    }
    if (event->key() == Qt::Key_X) {
    }
    if (event->key() == Qt::Key_Up) {
        mInput.UP = false;
    }
    if (event->key() == Qt::Key_Down) {
        mInput.DOWN = false;
    }
    if (event->key() == Qt::Key_Left) {
        mInput.LEFT = false;
    }
    if (event->key() == Qt::Key_Right) {
        mInput.RIGHT = false;
    }
    if (event->key() == Qt::Key_U) {
    }
    if (event->key() == Qt::Key_O) {
    }
}

void RenderWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        mInput.RMB = true;
    if (event->button() == Qt::LeftButton) {
        mInput.LMB = true;
        mBoat->Reset();
    }
    if (event->button() == Qt::MiddleButton)
        mInput.MMB = true;
}

void RenderWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
        mInput.RMB = false;
    if (event->button() == Qt::LeftButton)
        mInput.LMB = false;
    if (event->button() == Qt::MiddleButton)
        mInput.MMB = false;
}

void RenderWindow::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;

    //if RMB, change the speed of the camera
    if (mInput.RMB) {
        if (numDegrees.y() < 1)
            setCameraSpeed(0.001f);
        if (numDegrees.y() > 1)
            setCameraSpeed(-0.001f);
    }
    event->accept();
}

void RenderWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (mInput.RMB) {
        //Using mMouseXYlast as deltaXY so we don't need extra variables
        mMouseXlast = event->pos().x() - mMouseXlast;
        mMouseYlast = event->pos().y() - mMouseYlast;

        if (mMouseXlast != 0)
            mCurrentCamera->yaw(mCameraRotateSpeed * mMouseXlast);
        if (mMouseYlast != 0)
            mCurrentCamera->pitch(mCameraRotateSpeed * mMouseYlast);
    }
    mMouseXlast = event->pos().x();
    mMouseYlast = event->pos().y();
}
