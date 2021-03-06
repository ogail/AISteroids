#include "StateApproach.h"
#include "Ship.h"
#include "FSMAIControl.h"
#include "utility.h"
#include "GameSession.h"

using namespace cyclone;

//---------------------------------------------------------
void StateApproach::Update(float dt)
{
    //turn and then thrust towards closest asteroid
    FSMAIControl* parent = (FSMAIControl*)m_parent;
    GameObj* asteroid = parent->m_nearestAsteroid;
    Ship*    ship     = parent->m_ship;

	Vector3 deltaPos = asteroid->getPosition() - ship->getPosition();
	Vector3 astVelnormalised = asteroid->getVelocity();
	astVelnormalised.normalise();
    
	Vector3 futureAstPosition;

	//use braking vector if you're going too fast
	bool needToBrake = false;
    float speed		 = ship->getVelocity().magnitude();
    if(speed > parent->m_maxSpeed)
	{
		needToBrake = true;
		deltaPos	= -ship->getVelocity();
	}
	else
	{
		float dotVelocity = DOT(ship->UnitVectorVelocity(),asteroid->UnitVectorVelocity());

		//if the other guy is "to my front" and we're moving towards each other...
		Vector3 targetPos = asteroid->getPosition();
		if ((DOT(deltaPos,ship->UnitVectorVelocity()) < 0) || (dotVelocity > -0.93)) // magic number == about 21 degrees
		{
			Vector3 shipVel = ship->getVelocity();
			shipVel = shipVel.unit() * parent->m_maxSpeed;
			float combinedSpeed	  = (shipVel + asteroid->getVelocity()).magnitude();
			float predictionTime  = deltaPos.magnitude() / combinedSpeed;
			targetPos = asteroid->getPosition() + (asteroid->getVelocity() * predictionTime);
			deltaPos  = targetPos - ship->getPosition();

			Game.Clip(futureAstPosition);
		}
	}
	//sub off our current velocity, to get direction of wanted velocity
	deltaPos -= ship->getVelocity();
  
    //find new direction, and head to it
    float newDir	 = CALCDIR(deltaPos);
    float angDelta	 = CLAMPDIR180(newDir - ship->m_angle);
	bool canApproachInReverse = needToBrake || ship->GetShotLevel() != 0;

    if(fabsf(angDelta) < 3 || (fabsf(angDelta) > 177 && canApproachInReverse) )
    {
        //thrust
        ship->StopTurn();
        if(parent->m_nearestAsteroidDist > parent->m_nearestAsteroid->m_size + 20)
            fabsf(angDelta) < 3 ? ship->ThrustOn() : ship->ThrustReverse();
        else
            ship->ThrustOff();
    }
	else if(fabsf(angDelta) <= 90 || !canApproachInReverse)
    {
        //turn when facing forwards
		if(angDelta<0)
			ship->TurnRight();
		else if(angDelta>0)
			ship->TurnLeft();
    }
    else
    {
        //turn when facing rear
        if(angDelta > 0)
            ship->TurnRight();
        else if(angDelta < 0)
            ship->TurnLeft();
    }
    parent->m_target->setPosition(asteroid->getPosition());
    parent->m_targetDir = newDir;
    parent->m_debugTxt = "Approach";
}

//---------------------------------------------------------
int StateApproach::CheckTransitions()
{
    FSMAIControl* parent = (FSMAIControl*)m_parent;
    if(parent->m_willCollide)
        return FSM_STATE_EVADE;

    if(parent->m_powerupNear && parent->m_nearestAsteroidDist > 
	   parent->m_nearestPowerupDist && parent->m_ship->GetShotLevel() < MAX_SHOT_LEVEL)
        return FSM_STATE_GETPOWERUP;

    if(!parent->m_nearestAsteroid || parent->m_nearestAsteroidDist < APPROACH_DIST)
        return FSM_STATE_IDLE;

    return FSM_STATE_APPROACH;  
}

//---------------------------------------------------------
void StateApproach::Exit()
{
    if(((FSMAIControl*)m_parent)->m_ship)
    {
        ((FSMAIControl*)m_parent)->m_ship->ThrustOff();
        ((FSMAIControl*)m_parent)->m_ship->StopTurn();
    }
}