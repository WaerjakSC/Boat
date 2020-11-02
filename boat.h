#ifndef BOAT_H
#define BOAT_H

#include "visualobject.h"

class Boat : public VisualObject {
public:
    Boat(gsl::Vector3D startPosition);
    virtual void init() override;
    virtual void draw() override;
    void Rotate(float degrees);
    /**
     * Basic tick function ala Unreal Engine
     * @param deltaTime Smooth movement requires a deltatime variable,
     */
    void Tick(float deltaTime);
    /**
     * Function to take keyboard input and convert it into movement or rotation
     * @param key WASD keys
     * @param deltaTime Smooth rotation requires a deltatime variable,
     *  to account for FPS
     */
    void MoveInput(Qt::Key key, float deltaTime);
    /**
     * Reset the boat to starting position and speed
     */
    void Reset();
    gsl::Vector3D position();
    void SetPosition(gsl::Vector3D newPosition);

private:
    /**
     * Find the direction the boat is facing towards
     */
    void UpdateForwardVector();
    void UpdateRightVector();
    // Directional vectors
    gsl::Vector3D mForward{0.f, 0.f, -1.f};
    gsl::Vector3D mRight{1.f, 0.f, 0.f};
    gsl::Vector3D mUp{0.f, 1.f, 0.f};

    float mYaw{0.f};

    gsl::Vector3D mPosition{0.f, 0.f, 0.f};
    gsl::Vector3D mStartPosition{0.f, 10.f, 0.f};
    // Current speed
    float mSpeed{0.f};
    // How fast the boat gains speed
    float mAcceleration{30.f};
    float mRotateSpeed{150.f};

    // Force that works against the moving boat
    float mFriction{15.f};
    /**
     * Slow the boat to a stop if no other forces acting on it.
     * @param deltaTime
     */
    void ApplyFriction(float deltaTime);
};

#endif // BOAT_H
