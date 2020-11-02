#include "boat.h"

Boat::Boat(gsl::Vector3D startPosition) : mPosition(startPosition), mStartPosition(startPosition)
{
    mVertices.insert(mVertices.end(),
                     {
                         //Vertex data - normals not correct
                         Vertex{gsl::Vector3D(-0.5f, -0.5f, 0.5f), gsl::Vector3D(0.f, 0.f, 1.0f), gsl::Vector2D(0.f, 0.f)},  //Left low
                         Vertex{gsl::Vector3D(0.5f, -0.5f, 0.5f), gsl::Vector3D(0.f, 0.f, 1.0f), gsl::Vector2D(1.f, 0.f)},   //Right low
                         Vertex{gsl::Vector3D(0.0f, 0.5f, 0.0f), gsl::Vector3D(0.f, 0.f, 1.0f), gsl::Vector2D(0.5f, 0.5f)},  //Top
                         Vertex{gsl::Vector3D(0.0f, -0.5f, -0.5f), gsl::Vector3D(0.f, 0.f, 1.0f), gsl::Vector2D(0.5f, 0.5f)} //Back low
                     });

    mIndices.insert(mIndices.end(),
                    {0, 1, 2,
                     1, 3, 2,
                     3, 0, 2,
                     0, 3, 1});
    mMatrix.setToIdentity();
    mMatrix.translate(mPosition);
}
void Boat::init()
{
    initializeOpenGLFunctions();

    //Vertex Array Object - VAO
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    //Vertex Buffer Object to hold vertices - VBO
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(Vertex), mVertices.data(), GL_STATIC_DRAW);

    // 1rst attribute buffer : vertices
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // 2nd attribute buffer : colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 3rd attribute buffer : uvs
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    //Second buffer - holds the indices (Element Array Buffer - EAB):
    glGenBuffers(1, &mEAB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEAB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(GLuint), mIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}
void Boat::draw()
{
    glUseProgram(mMaterial.mShader->getProgram());
    glBindVertexArray(mVAO);
    mMaterial.mShader->transmitUniformData(&mMatrix, &mMaterial);
    glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, nullptr);
}
void Boat::Tick(float deltaTime)
{
    ApplyFriction(deltaTime);
    mPosition += mForward * mSpeed * deltaTime;
    mMatrix.setToIdentity();
    mMatrix.translate(mPosition);
    mMatrix.rotateY(-mYaw);
}

void Boat::MoveInput(Qt::Key key, float deltaTime)
{
    if (key == Qt::Key_W)
        mSpeed += mAcceleration * deltaTime;
    if (key == Qt::Key_S)
        mSpeed -= mAcceleration * deltaTime;
    if (key == Qt::Key_A)
        Rotate(-mRotateSpeed * deltaTime);
    if (key == Qt::Key_D)
        Rotate(mRotateSpeed * deltaTime);
}

void Boat::Reset()
{
    mSpeed = 0.f;
    mMatrix.setToIdentity();
    mPosition = mStartPosition;
    mYaw = 0.f;
    UpdateForwardVector();
}

gsl::Vector3D Boat::position()
{
    return mPosition;
}

void Boat::SetPosition(gsl::Vector3D newPosition)
{
    mPosition = newPosition;
    mMatrix.setToIdentity();
    mMatrix.translate(mPosition);
    mMatrix.rotateY(-mYaw);
}
void Boat::Rotate(float degrees)
{
    // rotate around mUp
    mYaw -= degrees;
    UpdateForwardVector();
}
void Boat::UpdateForwardVector()
{
    mRight = gsl::Vector3D(1.f, 0.f, 0.f);
    mRight.rotateY(mYaw);
    mRight.normalize();
    mForward = mUp ^ mRight;
    UpdateRightVector();
}

void Boat::UpdateRightVector()
{
    mRight = mForward ^ mUp;
    mRight.normalize();
}

void Boat::ApplyFriction(float deltaTime)
{
    if (mSpeed > 0) {
        mSpeed -= mFriction * deltaTime;
    }
    else if (mSpeed < 0) {
        mSpeed += mFriction * deltaTime;
    }
}
